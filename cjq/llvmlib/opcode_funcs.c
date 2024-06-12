/* A collection of C functions that perform the same actions as each
   opcode case found in execute.c > jq_next.
   
   All side-effects to cjq attributes are exactly the same.
*/
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../jq/src/bytecode.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/jq_parser.h"
#include "../jq/src/locfile.h"
#include "../jq/src/jv.h"
#include "../jq/src/common.h"
#include "../jq/src/bytecode.h"
#include "../jq/src/parser.h"
#include "../jq/src/builtin.h"
#include "../jq/src/util.h"
#include "../jq/src/linker.h"

#include "../bootstrap/cjq_bootstrap.h"

#define ON_BACKTRACK(op) ((op)+NUM_OPCODES)

struct closure {
  struct bytecode* bc;  // jq bytecode
  stack_ptr env;        // jq stack address of closed frame
};

// locals for any function called: either a closure or a local variable
union frame_entry {
  struct closure closure;
  jv localvar;
};

// jq function call frame
struct frame {
  struct bytecode* bc;      // jq bytecode for callee
  stack_ptr env;            // jq stack address of frame to return to
  stack_ptr retdata;        // jq stack address to unwind to on RET
  uint16_t* retaddr;        // jq bytecode return address
  union frame_entry entries[]; // nclosures + nlocals
};

static int frame_size(struct bytecode* bc) {
  return sizeof(struct frame) + sizeof(union frame_entry) * (bc->nclosures + bc->nlocals);
}

static struct frame* frame_current(struct jq_state* jq) {
  struct frame* fp = stack_block(&jq->stk, jq->curr_frame);

  stack_ptr next = *stack_block_next(&jq->stk, jq->curr_frame);
  if (next) {
    struct frame* fpnext = stack_block(&jq->stk, next);
    struct bytecode* bc = fpnext->bc;
    assert(fp->retaddr >= bc->code && fp->retaddr < bc->code + bc->codelen);
  } else {
    assert(fp->retaddr == 0);
  }
  return fp;
}

static stack_ptr frame_get_level(struct jq_state* jq, int level) {
  stack_ptr fr = jq->curr_frame;
  for (int i=0; i<level; i++) {
    struct frame* fp = stack_block(&jq->stk, fr);
    fr = fp->env;
  }
  return fr;
}

static jv* frame_local_var(struct jq_state* jq, int var, int level) {
  struct frame* fr = stack_block(&jq->stk, frame_get_level(jq, level));
  assert(var >= 0);
  assert(var < fr->bc->nlocals);
  return &fr->entries[fr->bc->nclosures + var].localvar;
}

static struct closure make_closure(struct jq_state* jq, uint16_t* pc) {
  uint16_t level = *pc++;
  uint16_t idx = *pc++;
  stack_ptr fridx = frame_get_level(jq, level);
  struct frame* fr = stack_block(&jq->stk, fridx);
  if (idx & ARG_NEWCLOSURE) {
    // A new closure closing the frame identified by level, and with
    // the bytecode body of the idx'th subfunction of that frame
    int subfn_idx = idx & ~ARG_NEWCLOSURE;
    assert(subfn_idx < fr->bc->nsubfunctions);
    struct closure cl = {fr->bc->subfunctions[subfn_idx],
                         fridx};
    return cl;
  } else {
    // A reference to a closure from the frame identified by level; copy
    // it as-is
    int closure = idx;
    assert(closure >= 0);
    assert(closure < fr->bc->nclosures);
    return fr->entries[closure].closure;
  }
}

static struct frame* frame_push(struct jq_state* jq, struct closure callee,
                                uint16_t* argdef, int nargs) {
  stack_ptr new_frame_idx = stack_push_block(&jq->stk, jq->curr_frame, frame_size(callee.bc));
  struct frame* new_frame = stack_block(&jq->stk, new_frame_idx);
  new_frame->bc = callee.bc;
  new_frame->env = callee.env;
  assert(nargs == new_frame->bc->nclosures);
  union frame_entry* entries = new_frame->entries;
  for (int i=0; i<nargs; i++) {
    entries->closure = make_closure(jq, argdef + i * 2);
    entries++;
  }
  for (int i=0; i<callee.bc->nlocals; i++) {
    entries->localvar = jv_invalid();
    entries++;
  }
  jq->curr_frame = new_frame_idx;
  return new_frame;
}

static void frame_pop(struct jq_state* jq) {
  assert(jq->curr_frame);
  struct frame* fp = frame_current(jq);
  if (stack_pop_will_free(&jq->stk, jq->curr_frame)) {
    int nlocals = fp->bc->nlocals;
    for (int i=0; i<nlocals; i++) {
      jv_free(*frame_local_var(jq, i, 0));
    }
  }
  jq->curr_frame = stack_pop_block(&jq->stk, jq->curr_frame, frame_size(fp->bc));
}

static void stack_push(jq_state *jq, jv val) {
  assert(jv_is_valid(val));
  jq->stk_top = stack_push_block(&jq->stk, jq->stk_top, sizeof(jv));
  jv* sval = stack_block(&jq->stk, jq->stk_top);
  *sval = val;
}

static jv stack_pop(jq_state *jq) {
  jv* sval = stack_block(&jq->stk, jq->stk_top);
  jv val = *sval;
  if (!stack_pop_will_free(&jq->stk, jq->stk_top)) {
    val = jv_copy(val);
  }
  jq->stk_top = stack_pop_block(&jq->stk, jq->stk_top, sizeof(jv));
  assert(jv_is_valid(val));
  return val;
}

// Like stack_pop(), but assert !stack_pop_will_free() and replace with
// jv_null() on the stack.
static jv stack_popn(jq_state *jq) {
  jv* sval = stack_block(&jq->stk, jq->stk_top);
  jv val = *sval;
  if (!stack_pop_will_free(&jq->stk, jq->stk_top)) {
    *sval = jv_null();
  }
  jq->stk_top = stack_pop_block(&jq->stk, jq->stk_top, sizeof(jv));
  assert(jv_is_valid(val));
  return val;
}

struct forkpoint {
  stack_ptr saved_data_stack;
  stack_ptr saved_curr_frame;
  int path_len, subexp_nest;
  jv value_at_path;
  uint16_t* return_address;
};

struct stack_pos {
  stack_ptr saved_data_stack, saved_curr_frame;
};

static struct stack_pos stack_get_pos(jq_state* jq) {
  struct stack_pos sp = {jq->stk_top, jq->curr_frame};
  return sp;
}

static void stack_save(jq_state *jq, uint16_t* retaddr, struct stack_pos sp){
  jq->fork_top = stack_push_block(&jq->stk, jq->fork_top, sizeof(struct forkpoint));
  struct forkpoint* fork = stack_block(&jq->stk, jq->fork_top);
  fork->saved_data_stack = jq->stk_top;
  fork->saved_curr_frame = jq->curr_frame;
  fork->path_len =
    jv_get_kind(jq->path) == JV_KIND_ARRAY ? jv_array_length(jv_copy(jq->path)) : 0;
  fork->value_at_path = jv_copy(jq->value_at_path);
  fork->subexp_nest = jq->subexp_nest;
  fork->return_address = retaddr;
  jq->stk_top = sp.saved_data_stack;
  jq->curr_frame = sp.saved_curr_frame;
}

static int path_intact(jq_state *jq, jv curr) {
  if (jq->subexp_nest == 0 && jv_get_kind(jq->path) == JV_KIND_ARRAY) {
    return jv_identical(curr, jv_copy(jq->value_at_path));
  } else {
    jv_free(curr);
    return 1;
  }
}

static void path_append(jq_state* jq, jv component, jv value_at_path) {
  if (jq->subexp_nest == 0 && jv_get_kind(jq->path) == JV_KIND_ARRAY) {
    int n1 = jv_array_length(jv_copy(jq->path));
    jq->path = jv_array_append(jq->path, component);
    int n2 = jv_array_length(jv_copy(jq->path));
    assert(n2 == n1 + 1);
    jv_free(jq->value_at_path);
    jq->value_at_path = value_at_path;
  } else {
    jv_free(component);
    jv_free(value_at_path);
  }
}

/* For f_getpath() */
static jv
_jq_path_append(jq_state *jq, jv v, jv p, jv value_at_path) {
  if (jq->subexp_nest != 0 ||
      jv_get_kind(jq->path) != JV_KIND_ARRAY ||
      !jv_is_valid(value_at_path)) {
    jv_free(v);
    jv_free(p);
    return value_at_path;
  }
  if (!jv_identical(v, jv_copy(jq->value_at_path))) {
    jv_free(p);
    return value_at_path;
  }
  if (jv_get_kind(p) == JV_KIND_ARRAY)
    jq->path = jv_array_concat(jq->path, p);
  else
    jq->path = jv_array_append(jq->path, p);
  jv_free(jq->value_at_path);
  return jv_copy(jq->value_at_path = value_at_path);
}

static uint16_t* stack_restore(jq_state *jq){
  while (!stack_pop_will_free(&jq->stk, jq->fork_top)) {
    if (stack_pop_will_free(&jq->stk, jq->stk_top)) {
      jv_free(stack_pop(jq));
    } else if (stack_pop_will_free(&jq->stk, jq->curr_frame)) {
      frame_pop(jq);
    } else {
      assert(0);
    }
  }

  if (jq->fork_top == 0) {
    return 0;
  }

  struct forkpoint* fork = stack_block(&jq->stk, jq->fork_top);
  uint16_t* retaddr = fork->return_address;
  jq->stk_top = fork->saved_data_stack;
  jq->curr_frame = fork->saved_curr_frame;
  int path_len = fork->path_len;
  if (jv_get_kind(jq->path) == JV_KIND_ARRAY) {
    assert(path_len >= 0);
    jq->path = jv_array_slice(jq->path, 0, path_len);
  } else {
    fork->path_len = 0;
  }
  jv_free(jq->value_at_path);
  jq->value_at_path = fork->value_at_path;
  jq->subexp_nest = fork->subexp_nest;
  jq->fork_top = stack_pop_block(&jq->stk, jq->fork_top, sizeof(struct forkpoint));
  return retaddr;
}

static void jq_reset(jq_state *jq) {
  while (stack_restore(jq)) {}

  assert(jq->stk_top == 0);
  assert(jq->fork_top == 0);
  assert(jq->curr_frame == 0);
  stack_reset(&jq->stk);
  jv_free(jq->error);
  jq->error = jv_null();

  jq->halted = 0;
  jv_free(jq->exit_code);
  jv_free(jq->error_message);
  if (jv_get_kind(jq->path) != JV_KIND_INVALID)
    jv_free(jq->path);
  jq->path = jv_null();
  jv_free(jq->value_at_path);
  jq->value_at_path = jv_null();
  jq->subexp_nest = 0;
}

static void set_error(jq_state *jq, jv value) {
  // Record so try/catch can find it.
  jv_free(jq->error);
  jq->error = value;
}

static void jq_error(cjq_state *cjq) {
   if (jq_halted(cjq->jq)) {
    // jq program invoked `halt` or `halt_error`
    jv exit_code = jq_get_exit_code(cjq->jq);
    if (!jv_is_valid(exit_code))
      *cjq->ret = JQ_OK;
    else if (jv_get_kind(exit_code) == JV_KIND_NUMBER)
      *cjq->ret = jv_number_value(exit_code);
    else
      *cjq->ret = JQ_ERROR_UNKNOWN;
    jv_free(exit_code);
    jv error_message = jq_get_error_message(cjq->jq);
    if (jv_get_kind(error_message) == JV_KIND_STRING) {
      // No prefix should be added to the output of `halt_error`.
      priv_fwrite(jv_string_value(error_message), jv_string_length_bytes(jv_copy(error_message)),
          stderr, *cjq->dumpopts & JV_PRINT_ISATTY);
    } else if (jv_get_kind(error_message) == JV_KIND_NULL) {
      // Halt with no output
    } else if (jv_is_valid(error_message)) {
      error_message = jv_dump_string(error_message, 0);
      fprintf(stderr, "%s\n", jv_string_value(error_message));
    } // else no message on stderr; use --debug-trace to see a message
    fflush(stderr);
    jv_free(error_message);
  } else if (jv_invalid_has_msg(jv_copy(*cjq->result))) {
    // Uncaught jq exception
    jv msg = jv_invalid_get_msg(jv_copy(*cjq->result));
    jv input_pos = jq_util_input_get_position(cjq->jq);
    if (jv_get_kind(msg) == JV_KIND_STRING) {
      fprintf(stderr, "jq: error (at %s): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    } else {
      msg = jv_dump_string(msg, 0);
      fprintf(stderr, "jq: error (at %s) (not a string): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    }
    *cjq->ret = JQ_ERROR_UNKNOWN;
    jv_free(input_pos);
    jv_free(msg);
  }
}

static void jq_print(cjq_state *cjq) {
   if ((*cjq->options & RAW_OUTPUT) && jv_get_kind(*cjq->result) == JV_KIND_STRING) {
      if (*cjq->options & ASCII_OUTPUT) {
        jv_dumpf(jv_copy(*cjq->result), stdout, JV_PRINT_ASCII);
      } else if ((*cjq->options & RAW_OUTPUT0) && strlen(jv_string_value(*cjq->result)) != (unsigned long)jv_string_length_bytes(jv_copy(*cjq->result))) {
        *cjq->result = jv_invalid_with_msg(jv_string(
              "Cannot dump a string containing NUL with --raw-output0 option"));
        jq_error(cjq);
      } else {
        priv_fwrite(jv_string_value(*cjq->result), jv_string_length_bytes(jv_copy(*cjq->result)),
            stdout, *cjq->dumpopts & JV_PRINT_ISATTY);
      }
      *cjq->ret = JQ_OK;
    } else {
      if (jv_get_kind(*cjq->result) == JV_KIND_FALSE || jv_get_kind(*cjq->result) == JV_KIND_NULL)
        *cjq->ret = JQ_OK_NULL_KIND;
      else
        *cjq->ret = JQ_OK;
      if (*cjq->options & SEQ)
        priv_fwrite("\036", 1, stdout, *cjq->dumpopts & JV_PRINT_ISATTY);
      jv_dump(*cjq->result, *cjq->dumpopts);  // JOHN: Where result of jq_next is printed
    }
    if (!(*cjq->options & RAW_NO_LF))
      priv_fwrite("\n", 1, stdout, *cjq->dumpopts & JV_PRINT_ISATTY);
    if (*cjq->options & RAW_OUTPUT0)
      priv_fwrite("\0", 1, stdout, *cjq->dumpopts & JV_PRINT_ISATTY);
    if (*cjq->options & UNBUFFERED_OUTPUT)
      fflush(stdout);
}

void lg_init(cjq_state* cjq) {
  /**
   * Below is everything that happens between loop guard
   * and start of switch statement in jq_next
  */
   if (cjq->jq->halted) {
      if (cjq->jq->debug_trace_enabled)
        printf("\t<halted>\n");
      *cjq->result = jv_invalid();
      return;
    }
   *cjq->opcode = *cjq->pc;
   *cjq->raising = 0;

   if (cjq->jq->debug_trace_enabled) {
    dump_operation(frame_current(cjq->jq)->bc, cjq->pc);
    printf("\t");
    const struct opcode_description* opdesc = opcode_describe(*cjq->opcode);
    stack_ptr param = 0;
    if (!*cjq->backtracking) {
      int stack_in = opdesc->stack_in;
      if (stack_in == -1) stack_in = cjq->pc[1];
      param = cjq->jq->stk_top;
      for (int i=0; i<stack_in; i++) {
        if (i != 0) {
          printf(" | ");
          param = *stack_block_next(&cjq->jq->stk, param);
        }
        if (!param) break;
        jv_dump(jv_copy(*(jv*)stack_block(&cjq->jq->stk, param)), JV_PRINT_REFCOUNT);
        //printf("<%d>", jv_get_refcnt(param->val));
        //printf(" -- ");
        //jv_dump(jv_copy(jq->path), 0);
      }
      if (cjq->jq->debug_trace_enabled & JQ_DEBUG_TRACE_DETAIL) {
        while ((param = *stack_block_next(&cjq->jq->stk, param))) {
          printf(" || ");
          jv_dump(jv_copy(*(jv*)stack_block(&cjq->jq->stk, param)), JV_PRINT_REFCOUNT);
        }
      }
    } else {
      printf("\t<backtracking>");
    }

    printf("\n");
  }
   if (*cjq->backtracking) {
      *cjq->opcode = ON_BACKTRACK(*cjq->opcode);
      *cjq->backtracking = 0;
      *cjq->raising = !jv_is_valid(cjq->jq->error);
    }
    cjq->pc++;
}

void _do_backtrack(cjq_state *cjq) {
   cjq->pc = stack_restore(cjq->jq);
   if (!cjq->pc) {
      if (!jv_is_valid(cjq->jq->error)) {
         jv error = cjq->jq->error;
         cjq->jq->error = jv_null();
         *cjq->result = error;
      }
      *cjq->result = jv_invalid();
   }
   *cjq->backtracking = 1;
}

void _jq_halt(void* cjq) {
  cjq_state* pcjq = (cjq_state*)cjq;
  if (jq_halted(pcjq->jq)) {
    // jq program invoked `halt` or `halt_error`
    jv exit_code = jq_get_exit_code(pcjq->jq);
    if (!jv_is_valid(exit_code))
      *pcjq->ret = JQ_OK;
    else if (jv_get_kind(exit_code) == JV_KIND_NUMBER)
      *pcjq->ret = jv_number_value(exit_code);
    else
      *pcjq->ret = JQ_ERROR_UNKNOWN;
    jv_free(exit_code);
    jv error_message = jq_get_error_message(pcjq->jq);
    if (jv_get_kind(error_message) == JV_KIND_STRING) {
      // No prefix should be added to the output of `halt_error`.
      priv_fwrite(jv_string_value(error_message), jv_string_length_bytes(jv_copy(error_message)),
          stderr, *pcjq->dumpopts & JV_PRINT_ISATTY);
    } else if (jv_get_kind(error_message) == JV_KIND_NULL) {
      // Halt with no output
    } else if (jv_is_valid(error_message)) {
      error_message = jv_dump_string(error_message, 0);
      fprintf(stderr, "%s\n", jv_string_value(error_message));
    } // else no message on stderr; use --debug-trace to see a message
    fflush(stderr);
    jv_free(error_message);
  } else if (jv_invalid_has_msg(jv_copy(*pcjq->result))) {
    // Uncaught jq exception
    jv msg = jv_invalid_get_msg(jv_copy(*pcjq->result));
    jv input_pos = jq_util_input_get_position(pcjq->jq);
    if (jv_get_kind(msg) == JV_KIND_STRING) {
      fprintf(stderr, "jq: error (at %s): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    } else {
      msg = jv_dump_string(msg, 0);
      fprintf(stderr, "jq: error (at %s) (not a string): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    }
    *pcjq->ret = JQ_ERROR_UNKNOWN;
    jv_free(input_pos);
    jv_free(msg);
  }
}

void _init_jq_next(void* cjq) {
  /**
   * Below should be equivalent logic to what happens 
   * between start of jq_next and start of while loop
   * in jq_next.
  */
  cjq_state* pcjq = (cjq_state*)cjq;
  jv_nomem_handler(pcjq->jq->nomem_handler, pcjq->jq->nomem_handler_data);
  pcjq->pc = stack_restore(pcjq->jq);
  assert(pcjq->pc);
  *pcjq->backtracking = !pcjq->jq->initial_execution;
  pcjq->jq->initial_execution = 0;
  assert(jv_get_kind(pcjq->jq->error) == JV_KIND_NULL);
}

static void _free_prev_input_val(jv* p) {
  if (p) {
    jv_free(*p);
    free(p);
    p = NULL;
  }
}

void _get_next_input(void* cjq) {
  /**
   * Below should be equivalent logic to what happens
   * between if-else block where process() is called
   * and start of while-loop in process()
  */
  cjq_state* pcjq = (cjq_state*)cjq;
  
  // Initialize value to prevent usage of uninitialized memory
  jv value = jv_invalid();

  if (*pcjq->options & PROVIDE_NULL) {
    _free_prev_input_val(pcjq->value);
    pcjq->value = malloc(sizeof(jv));
    if (!pcjq->value) {
      perror("Failed to allocate memory for jv_null");
      exit(EXIT_FAILURE);
    }
    *pcjq->value = jv_null();
  } else {
    if (jq_util_input_errors(pcjq->input_state) == 0 &&
        (jv_is_valid(value = jq_util_input_next_input(pcjq->input_state)) || 
         jv_invalid_has_msg(jv_copy(value)))) {
      
      _free_prev_input_val(pcjq->value);
      pcjq->value = malloc(sizeof(jv));
      if (!pcjq->value) {
        perror("Failed to allocate memory for jv value");
        jv_free(value);
        exit(EXIT_FAILURE);
      }
      *pcjq->value = value;
      if (!jv_is_valid(*pcjq->value)) {
        // Parse error
        jv msg = jv_invalid_get_msg(*pcjq->value);
        if (!(*pcjq->options & SEQ)) {
          *pcjq->ret = JQ_ERROR_UNKNOWN;
          fprintf(stderr, "jq: parse error: %s\n", jv_string_value(msg));
          jv_free(msg);
          jv_free(value);
          exit(EXIT_FAILURE);
        }
        // --seq -> errors are not fatal
        fprintf(stderr, "jq: ignoring parse error: %s\n", jv_string_value(msg));
        jv_free(msg);
      }
    } else {
      jv_free(value);
      exit(EXIT_FAILURE);
    }
  }
  *pcjq->ret = JQ_OK_NO_OUTPUT;
  jq_start(pcjq->jq, *pcjq->value, *pcjq->jq_flags);
}

void _update_result_state(void* cjq) {
  /**
   * Below should be equivalent logic to what happens
   * between return from process() and the end
   * of the if-else block where process() is called
  */
  cjq_state* pcjq = (cjq_state*)cjq; 
  if (!(*pcjq->options & PROVIDE_NULL)) {
    if (jv_is_valid(*pcjq->value)) {
      if (*pcjq->ret <= 0 && *pcjq->ret != JQ_OK_NO_OUTPUT)
        *pcjq->last_result = (*pcjq->ret != JQ_OK_NULL_KIND);
    } else {
      // Parse error
      jv msg = jv_invalid_get_msg(*pcjq->value);
      if (!(*pcjq->options & SEQ)) {
        *pcjq->ret = JQ_ERROR_UNKNOWN;
        fprintf(stderr, "jq: parse error: %s\n", jv_string_value(msg));
        jv_free(msg);
        jv_free(*pcjq->value);
        return;
      }
      // --seq -> errors are not fatal
      fprintf(stderr, "jq: ignoring parse error: %s\n", jv_string_value(msg));
      jv_free(msg);
    }
  }
}

void _opcode_LOADK(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = jv_array_get(jv_copy(frame_current(pcjq->jq)->bc->constants), *pcjq->pc++);
  assert(jv_is_valid(v));
  jv_free(stack_pop(pcjq->jq));
  stack_push(pcjq->jq, v);
 }

void _opcode_DUP(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, jv_copy(v));
  stack_push(pcjq->jq, v);
 }

void _opcode_DUPN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_popn(pcjq->jq);
  stack_push(pcjq->jq, jv_copy(v));
  stack_push(pcjq->jq, v);
 }

void _opcode_DUP2(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv keep = stack_pop(pcjq->jq);
  jv v = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, jv_copy(v));
  stack_push(pcjq->jq, keep);
  stack_push(pcjq->jq, v);
 }

void _opcode_PUSHK_UNDER(void* cjq) {
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = jv_array_get(jv_copy(frame_current(pcjq->jq)->bc->constants), *pcjq->pc++);
  assert(jv_is_valid(v));
  jv v2 = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, v);
  stack_push(pcjq->jq, v2);
}

void _opcode_POP(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv_free(stack_pop(pcjq->jq));
 }

void _opcode_LOADV(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  if (pcjq->jq->debug_trace_enabled) {
    printf("V%d = ", v);
    jv_dump(jv_copy(*var), JV_PRINT_REFCOUNT);
    printf("\n");
  }
  jv_free(stack_pop(pcjq->jq));
  stack_push(pcjq->jq, jv_copy(*var));
 }

void _opcode_LOADVN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  if (pcjq->jq->debug_trace_enabled) {
    printf("V%d = ", v);
    jv_dump(jv_copy(*var), JV_PRINT_REFCOUNT);
    printf("\n");
  }
  jv_free(stack_popn(pcjq->jq));

  // This `stack_push()` invalidates the `var` reference, so
  stack_push(pcjq->jq, *var);
  // we have to re-resolve `var` before we can set it to null
  var = frame_local_var(pcjq->jq, v, level);
  *var = jv_null();
 }

void _opcode_STOREV(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  if (!*pcjq->fallthrough)
    lg_init(pcjq);
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  jv val = stack_pop(pcjq->jq);
  if (pcjq->jq->debug_trace_enabled) {
    printf("V%d = ", v);
    jv_dump(jv_copy(val), 0);
    printf(" (%d)\n", jv_get_refcnt(val));
  }
  jv_free(*var);
  *var = val;
  *pcjq->fallthrough = 0;
 }

void _opcode_STORE_GLOBAL(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  // Get the constant
  jv val = jv_array_get(jv_copy(frame_current(pcjq->jq)->bc->constants), *pcjq->pc++);
  assert(jv_is_valid(val));

  // Store the var
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  if (pcjq->jq->debug_trace_enabled) {
    printf("V%d = ", v);
    jv_dump(jv_copy(val), 0);
    printf(" (%d)\n", jv_get_refcnt(val));
  }
  jv_free(*var);
  *var = val;
 }

void _opcode_INDEX(void* cjq) {
 _opcode_INDEX_OPT(cjq);
}

void _opcode_INDEX_OPT(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv t = stack_pop(pcjq->jq);
  jv k = stack_pop(pcjq->jq);
  // detect invalid path expression like path(reverse | .a)
  if (!path_intact(pcjq->jq, jv_copy(t))) {
    char keybuf[15];
    char objbuf[30];
    jv msg = jv_string_fmt(
        "Invalid path expression near attempt to access element %s of %s",
        jv_dump_string_trunc(k, keybuf, sizeof(keybuf)),
        jv_dump_string_trunc(t, objbuf, sizeof(objbuf)));
    set_error(pcjq->jq, jv_invalid_with_msg(msg));
    _do_backtrack(pcjq);
    return;
  }
  jv v = jv_get(t, jv_copy(k));
  if (jv_is_valid(v)) {
    path_append(pcjq->jq, k, jv_copy(v));
    stack_push(pcjq->jq, v);
  } else {
    jv_free(k);
    if (*pcjq->opcode == INDEX) 
      set_error(pcjq->jq, v);
    else
      jv_free(v);
      _do_backtrack(pcjq);
      return;
  }
  return;
 }

void _opcode_EACH(void* cjq) { 
 _opcode_EACH_OPT(cjq);
 }

void _opcode_BACKTRACK_EACH(void* cjq) { 
 _opcode_BACKTRACK_EACH_OPT(cjq);
 }

void _opcode_EACH_OPT(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv container = stack_pop(pcjq->jq);
  // detect invalid path expression like path(reverse | .[])
  if (!path_intact(pcjq->jq, jv_copy(container))) {
    char errbuf[30];
    jv msg = jv_string_fmt(
        "Invalid path expression near attempt to iterate through %s",
        jv_dump_string_trunc(container, errbuf, sizeof(errbuf)));
    set_error(pcjq->jq, jv_invalid_with_msg(msg));
    _do_backtrack(pcjq);
    return;
  }
  stack_push(pcjq->jq, container);
  stack_push(pcjq->jq, jv_number(-1));
  // JQ_FALLTHROUGH;
  *pcjq->fallthrough = 1;
 _opcode_BACKTRACK_EACH(cjq);
 }

void _opcode_BACKTRACK_EACH_OPT(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  if (!*pcjq->fallthrough)
    lg_init(pcjq);
  int idx = jv_number_value(stack_pop(pcjq->jq));
  jv container = stack_pop(pcjq->jq);
  int keep_going, is_last = 0;
  jv key, value;
  if (jv_get_kind(container) == JV_KIND_ARRAY) {
    if (*pcjq->opcode == EACH || *pcjq->opcode == EACH_OPT) idx = 0;
    else idx = idx + 1;
    int len = jv_array_length(jv_copy(container));
    keep_going = idx < len;
    is_last = idx == len - 1;
    if (keep_going) {
      key = jv_number(idx);
      value = jv_array_get(jv_copy(container), idx);
    }
  } else if (jv_get_kind(container) == JV_KIND_OBJECT) {
    if (*pcjq->opcode == EACH || *pcjq->opcode == EACH_OPT) idx = jv_object_iter(container);
    else idx = jv_object_iter_next(container, idx);
    keep_going = jv_object_iter_valid(container, idx);
    if (keep_going) {
      key = jv_object_iter_key(container, idx);
      value = jv_object_iter_value(container, idx);
    }
  } else {
    assert(*pcjq->opcode == EACH || *pcjq->opcode == EACH_OPT);
    if (*pcjq->opcode == EACH) {
      char errbuf[15];
      set_error(pcjq->jq,
                jv_invalid_with_msg(jv_string_fmt("Cannot iterate over %s (%s)",
                                                  jv_kind_name(jv_get_kind(container)),
                                                  jv_dump_string_trunc(jv_copy(container), errbuf, sizeof(errbuf)))));
    }
    keep_going = 0;
  }

  if (!keep_going || *pcjq->raising) {
    if (keep_going)
      jv_free(value);
    jv_free(container);
    _do_backtrack(cjq);
  } else if (is_last) {
    // we don't need to make a backtrack point
    jv_free(container);
    path_append(pcjq->jq, key, jv_copy(value));
    stack_push(pcjq->jq, value);
  } else {
    struct stack_pos spos = stack_get_pos(pcjq->jq);
    stack_push(pcjq->jq, container);
    stack_push(pcjq->jq, jv_number(idx));
    stack_save(pcjq->jq, pcjq->pc - 1, spos);
    path_append(pcjq->jq, key, jv_copy(value));
    stack_push(pcjq->jq, value);
  }

  *pcjq->fallthrough = 0;
 }

void _opcode_FORK(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  stack_save(pcjq->jq, pcjq->pc - 1, stack_get_pos(pcjq->jq));
  pcjq->pc++; // skip offset this time
 }

void _opcode_BACKTRACK_FORK(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  if (*pcjq->raising) {
    _do_backtrack(pcjq);
    return;
  }
  uint16_t offset = *pcjq->pc++;
  pcjq->pc += offset;
 }

void _opcode_TRY_BEGIN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  stack_save(pcjq->jq, pcjq->pc - 1, stack_get_pos(pcjq->jq));
  pcjq->pc++; // skip handler offset this time
 }

void _opcode_BACKTRACK_TRY_BEGIN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  if (!*pcjq->raising) {
    /*
      * `try EXP ...` -- EXP backtracked (e.g., EXP was `empty`), so we
      * backtrack more:
      */
    jv_free(stack_pop(pcjq->jq));
    _do_backtrack(pcjq);
    return;
  }
  /*
    * Else `(try EXP ... ) | EXP2` raised an error.
    *
    * If the error was wrapped in another error, then that means EXP2 raised
    * the error.  We unwrap it and re-raise it as it wasn't raised by EXP.
    *
    * See commentary in gen_try().
    */
  jv e = jv_invalid_get_msg(jv_copy(pcjq->jq->error));
  if (!jv_is_valid(e) && jv_invalid_has_msg(jv_copy(e))) {
    set_error(pcjq->jq, e);
    _do_backtrack(pcjq);
    return;
  }
  jv_free(e);

  /*
    * Else we caught an error containing a non-error value, so we jump to
    * the handler.
    *
    * See commentary in gen_try().
    */
  uint16_t offset = *pcjq->pc++;
  jv_free(stack_pop(pcjq->jq)); // free the input
  stack_push(pcjq->jq, jv_invalid_get_msg(pcjq->jq->error));  // push the error's message
  pcjq->jq->error = jv_null();
  pcjq->pc += offset;
 }

void _opcode_TRY_END(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  stack_save(pcjq->jq, pcjq->pc - 1, stack_get_pos(pcjq->jq));
 }

void _opcode_BACKTRACK_TRY_END(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  // Wrap the error so the matching TRY_BEGIN doesn't catch it
  if (*pcjq->raising)
    set_error(pcjq->jq, jv_invalid_with_msg(jv_copy(pcjq->jq->error)));
  _do_backtrack(pcjq);
  return;
 }

void _opcode_JUMP(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t offset = *pcjq->pc++;
  pcjq->pc += offset;
 }

void _opcode_JUMP_F(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t offset = *pcjq->pc++;
  jv t = stack_pop(pcjq->jq);
  jv_kind kind = jv_get_kind(t);
  if (kind == JV_KIND_FALSE || kind == JV_KIND_NULL) {
    pcjq->pc += offset;
  }
  stack_push(pcjq->jq, t); // FIXME do this better
 }

void _opcode_BACKTRACK(void* cjq) { 
   lg_init((cjq_state*)cjq);
   _do_backtrack((cjq_state*)cjq);
   return;
 }
 
void _opcode_APPEND(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_pop(pcjq->jq);
  uint16_t level = *pcjq->pc++;
  uint16_t vidx = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, vidx, level);
  assert(jv_get_kind(*var) == JV_KIND_ARRAY);
  *var = jv_array_append(*var, v);
 }
 
void _opcode_INSERT(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv stktop = stack_pop(pcjq->jq);
  jv v = stack_pop(pcjq->jq);
  jv k = stack_pop(pcjq->jq);
  jv objv = stack_pop(pcjq->jq);
  assert(jv_get_kind(objv) == JV_KIND_OBJECT);
  if (jv_get_kind(k) == JV_KIND_STRING) {
    stack_push(pcjq->jq, jv_object_set(objv, k, v));
    stack_push(pcjq->jq, stktop);
  } else {
    char errbuf[15];
    set_error(pcjq->jq, jv_invalid_with_msg(jv_string_fmt("Cannot use %s (%s) as object key",
                                                    jv_kind_name(jv_get_kind(k)),
                                                    jv_dump_string_trunc(jv_copy(k), errbuf, sizeof(errbuf)))));
    jv_free(stktop);
    jv_free(v);
    jv_free(k);
    jv_free(objv);
    _do_backtrack(pcjq);
    return;
  }
 }
 
void _opcode_RANGE(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  jv max = stack_pop(pcjq->jq);
  if (*pcjq->raising) {
    jv_free(max);
    _do_backtrack(pcjq);
    return;
  } 
  if (jv_get_kind(*var) != JV_KIND_NUMBER ||
      jv_get_kind(max) != JV_KIND_NUMBER) {
    set_error(pcjq->jq, jv_invalid_with_msg(jv_string_fmt("Range bounds must be numeric")));
    jv_free(max);
    _do_backtrack(pcjq);
    return;
  } else if (jv_number_value(*var) >= jv_number_value(max)) {
    /* finished iterating */
    jv_free(max);
    _do_backtrack(pcjq);
    return;
  } else {
    jv curr = *var;
    *var = jv_number(jv_number_value(*var) + 1);

    struct stack_pos spos = stack_get_pos(pcjq->jq);
    stack_push(pcjq->jq, max);
    stack_save(pcjq->jq, pcjq->pc - 3, spos);

    stack_push(pcjq->jq, curr);
  }
 }
 
void _opcode_BACKTRACK_RANGE(void* cjq) { 
 _opcode_RANGE(cjq);
 }

void _opcode_SUBEXP_BEGIN(void* cjq) {
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, jv_copy(v));
  stack_push(pcjq->jq, v);
  pcjq->jq->subexp_nest++;
  return;
}

void _opcode_SUBEXP_END(void* cjq) {
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  assert(pcjq->jq->subexp_nest > 0);
  pcjq->jq->subexp_nest--;
  jv a = stack_pop(pcjq->jq);
  jv b = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, a);
  stack_push(pcjq->jq, b);
  return;
}
 
void _opcode_PATH_BEGIN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_pop(pcjq->jq);
  stack_push(pcjq->jq, pcjq->jq->path);

  stack_save(pcjq->jq, pcjq->pc - 1, stack_get_pos(pcjq->jq));

  stack_push(pcjq->jq, jv_number(pcjq->jq->subexp_nest));
  stack_push(pcjq->jq, pcjq->jq->value_at_path);
  stack_push(pcjq->jq, jv_copy(v));

  pcjq->jq->path = jv_array();
  pcjq->jq->value_at_path = v; // next INDEX operation must index into v
  pcjq->jq->subexp_nest = 0;
 }
 
void _opcode_BACKTRACK_PATH_BEGIN(void* cjq) { 
 _opcode_BACKTRACK_PATH_END(cjq);
 }
  
void _opcode_PATH_END(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = stack_pop(pcjq->jq);
  // detect invalid path expression like path(.a | reverse)
  if (!path_intact(pcjq->jq, jv_copy(v))) {
    char errbuf[30];
    jv msg = jv_string_fmt(
        "Invalid path expression with result %s",
        jv_dump_string_trunc(v, errbuf, sizeof(errbuf)));
    set_error(pcjq->jq, jv_invalid_with_msg(msg));
    _do_backtrack(pcjq);
    return;
  }
  jv_free(v); // discard value, only keep path

  jv old_value_at_path = stack_pop(pcjq->jq);
  int old_subexp_nest = (int)jv_number_value(stack_pop(pcjq->jq));

  jv path = pcjq->jq->path;
  pcjq->jq->path = stack_pop(pcjq->jq);

  struct stack_pos spos = stack_get_pos(pcjq->jq);
  stack_push(pcjq->jq, jv_copy(path));
  stack_save(pcjq->jq, pcjq->pc - 1, spos);

  stack_push(pcjq->jq, path);
  pcjq->jq->subexp_nest = old_subexp_nest;
  jv_free(pcjq->jq->value_at_path);
  pcjq->jq->value_at_path = old_value_at_path;
 }
  
void _opcode_BACKTRACK_PATH_END(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv_free(pcjq->jq->path);
  pcjq->jq->path = stack_pop(pcjq->jq);
  _do_backtrack(pcjq);
  return;
 }

void _opcode_CALL_BUILTIN(void* cjq) {
  jv cfunc_input[MAX_CFUNCTION_ARGS];
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  int nargs = *pcjq->pc++;
  jv top = stack_pop(pcjq->jq);
  jv *in = cfunc_input;
  in[0] = top;
  for (int i = 1; i < nargs; i++) {
    in[i] = stack_pop(pcjq->jq);
  }
  struct cfunction* function = &frame_current(pcjq->jq)->bc->globals->cfunctions[*pcjq->pc++];
  switch (function->nargs) {
  case 1: top = ((jv (*)(jq_state *, jv))function->fptr)(pcjq->jq, in[0]); break;
  case 2: top = ((jv (*)(jq_state *, jv, jv))function->fptr)(pcjq->jq, in[0], in[1]); break;
  case 3: top = ((jv (*)(jq_state *, jv, jv, jv))function->fptr)(pcjq->jq, in[0], in[1], in[2]); break;
  case 4: top = ((jv (*)(jq_state *, jv, jv, jv, jv))function->fptr)(pcjq->jq, in[0], in[1], in[2], in[3]); break;
  case 5: top = ((jv (*)(jq_state *, jv, jv, jv, jv, jv))function->fptr)(pcjq->jq, in[0], in[1], in[2], in[3], in[4]); break;
  // FIXME: a) up to 7 arguments (input + 6), b) should assert
  // because the compiler should not generate this error.
  default: *pcjq->result = jv_invalid_with_msg(jv_string("Function takes too many arguments"));
  return;
  }

  if (jv_is_valid(top)) {
    stack_push(pcjq->jq, top);
  } else if (jv_invalid_has_msg(jv_copy(top))) {
    set_error(pcjq->jq, top);
    _do_backtrack(pcjq);
    return;
  } else {
    // C-coded function returns invalid w/o msg? -> backtrack, as if
    // it had returned `empty`
    _do_backtrack(pcjq);
    return;
  }
  return;
}
  
void _opcode_CALL_JQ(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  /*
      * Bytecode layout here:
      *
      *  CALL_JQ
      *  <nclosures>                       (i.e., number of call arguments)
      *  <callee closure>                  (what we're calling)
      *  <nclosures' worth of closures>    (frame reference + code pointer)
      *
      *  <next instruction (to return to)>
      *
      * Each closure consists of two uint16_t values: a "level"
      * identifying the frame to be closed over, and an index.
      *
      * The level is a relative number of call frames reachable from
      * the currently one; 0 -> current frame, 1 -> previous frame, and
      * so on.
      *
      * The index is either an index of the closed frame's subfunctions
      * or of the closed frame's parameter closures.  If the latter,
      * that closure will be passed, else the closed frame's pointer
      * and the subfunction's code will form the closure to be passed.
      *
      * See make_closure() for more information.
  */
  jv input = stack_pop(pcjq->jq);
  uint16_t nclosures = *pcjq->pc++;
  uint16_t* retaddr = pcjq->pc + 2 + nclosures*2;
  stack_ptr retdata = pcjq->jq->stk_top;
  struct frame* new_frame;
  struct closure cl = make_closure(pcjq->jq, pcjq->pc);
  if (*pcjq->opcode == TAIL_CALL_JQ) {
    retaddr = frame_current(pcjq->jq)->retaddr;
    retdata = frame_current(pcjq->jq)->retdata;
    frame_pop(pcjq->jq);
  }
  new_frame = frame_push(pcjq->jq, cl, pcjq->pc + 2, nclosures);
  new_frame->retdata = retdata;
  new_frame->retaddr = retaddr;
  pcjq->pc = new_frame->bc->code;
  stack_push(pcjq->jq, input);
 }

void _opcode_RET(void* cjq) {
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv value = stack_pop(pcjq->jq);
  assert(pcjq->jq->stk_top == frame_current(pcjq->jq)->retdata);
  uint16_t* retaddr = frame_current(pcjq->jq)->retaddr;
  if (retaddr) {
    // function return
    pcjq->pc = retaddr;
    frame_pop(pcjq->jq);
  } else {
    // top-level return, yielding value
    struct stack_pos spos = stack_get_pos(pcjq->jq);
    stack_push(pcjq->jq, jv_null());
    stack_save(pcjq->jq, pcjq->pc - 1, spos);
    // pcjq->result = malloc(sizeof(jv));
    *pcjq->result = value;
    jq_print(cjq);
    return;
  }
  stack_push(pcjq->jq, value);
  return;
}

void _opcode_BACKTRACK_RET(void* cjq) {
   lg_init((cjq_state*)cjq);
   _do_backtrack((cjq_state*)cjq);
   return;
}

void  _opcode_TAIL_CALL_JQ(void* cjq) { 
  _opcode_CALL_JQ(cjq);
 }

void _opcode_TOP(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
 }

void _opcode_GENLABEL(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  stack_push(pcjq->jq, JV_OBJECT(jv_string("__jq"), jv_number(pcjq->jq->next_label++)));
 }

void _opcode_DESTRUCTURE_ALT(void* cjq) { 
 _opcode_FORK(cjq);
 }

void _opcode_BACKTRACK_DESTRUCTURE_ALT(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  if (jv_is_valid(pcjq->jq->error)) {
    // `try EXP ...` backtracked here (no value, `empty`), so we backtrack more
    jv_free(stack_pop(pcjq->jq));
    _do_backtrack(pcjq);
    return;
  }
  // `try EXP ...` exception caught in EXP
  // DESTRUCTURE_ALT doesn't want the error message on the stack,
  // as we would just want to throw it away anyway.
  if (*pcjq->opcode != ON_BACKTRACK(DESTRUCTURE_ALT)) {
    jv_free(stack_pop(pcjq->jq)); // free the input
    stack_push(pcjq->jq, jv_invalid_get_msg(pcjq->jq->error));  // push the error's message
  } else {
    jv_free(pcjq->jq->error);
  }
  pcjq->jq->error = jv_null();
  uint16_t offset = *pcjq->pc++;
  pcjq->pc += offset;
 }

void _opcode_STOREVN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  stack_save(pcjq->jq, pcjq->pc - 1, stack_get_pos(pcjq->jq));
  // JQ_FALLTHROUGH;
  *pcjq->fallthrough = 1;
 _opcode_STOREV(cjq);
 }
 
void _opcode_BACKTRACK_STOREVN(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  uint16_t level = *pcjq->pc++;
  uint16_t v = *pcjq->pc++;
  jv* var = frame_local_var(pcjq->jq, v, level);
  jv_free(*var);
  *var = jv_null();
  _do_backtrack(pcjq);
  return;
 }

void _opcode_ERRORK(void* cjq) { 
  cjq_state* pcjq = (cjq_state*)cjq;
  lg_init(pcjq);
  jv v = jv_array_get(jv_copy(frame_current(pcjq->jq)->bc->constants), *pcjq->pc++);
  set_error(pcjq->jq, jv_invalid_with_msg(v));
  _do_backtrack(pcjq);
  return;
 }
