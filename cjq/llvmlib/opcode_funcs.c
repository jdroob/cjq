/* A collection of C functions that perform the same actions as each
   opcode case found in execute.c > jq_next.
   
   All side-effects to cjq_state attributes are exactly the same.
*/
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/stat.h>

#include "../jq/src/exec_stack.h"
#include "../jq/src/bytecode.h"

#include "../jq/src/jv_alloc.h"
#include "../jq/src/jq_parser.h"
#include "../jq/src/locfile.h"
#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../jq/src/parser.h"
#include "../jq/src/builtin.h"
#include "../jq/src/util.h"
#include "../jq/src/linker.h"

#include "../frontend/cjq_frontend.h"

struct jq_state {
  void (*nomem_handler)(void *);
  void *nomem_handler_data;
  struct bytecode* bc;

  jq_msg_cb err_cb;
  void *err_cb_data;
  jv error;

  struct stack stk;
  stack_ptr curr_frame;
  stack_ptr stk_top;
  stack_ptr fork_top;

  jv path;
  jv value_at_path;
  int subexp_nest;
  int debug_trace_enabled;
  int initial_execution;
  unsigned next_label;

  int halted;
  jv exit_code;
  jv error_message;

  jv attrs;
  jq_input_cb input_cb;
  void *input_cb_data;
  jq_msg_cb debug_cb;
  void *debug_cb_data;
  jq_msg_cb stderr_cb;
  void *stderr_cb_data;
};

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

enum {
  SLURP                 = 1,
  RAW_INPUT             = 2,
  PROVIDE_NULL          = 4,
  RAW_OUTPUT            = 8,
  RAW_OUTPUT0           = 16,
  ASCII_OUTPUT          = 32,
  COLOR_OUTPUT          = 64,
  NO_COLOR_OUTPUT       = 128,
  SORTED_OUTPUT         = 256,
  FROM_FILE             = 512,
  RAW_NO_LF             = 1024,
  UNBUFFERED_OUTPUT     = 2048,
  EXIT_STATUS           = 4096,
  SEQ                   = 16384,
  RUN_TESTS             = 32768,
  /* debugging only */
  DUMP_DISASM           = 65536,
};

enum {
    JQ_OK              =  0,
    JQ_OK_NULL_KIND    = -1, /* exit 0 if --exit-status is not set*/
    JQ_ERROR_SYSTEM    =  2,
    JQ_ERROR_COMPILE   =  3,
    JQ_OK_NO_OUTPUT    = -4, /* exit 0 if --exit-status is not set*/
    JQ_ERROR_UNKNOWN   =  5,
};

static void jq_error(compiled_jq_state *cjq_state) {
   if (jq_halted(cjq_state->jq)) {
    // jq program invoked `halt` or `halt_error`
    jv exit_code = jq_get_exit_code(cjq_state->jq);
    if (!jv_is_valid(exit_code))
      *cjq_state->ret = JQ_OK;
    else if (jv_get_kind(exit_code) == JV_KIND_NUMBER)
      *cjq_state->ret = jv_number_value(exit_code);
    else
      *cjq_state->ret = JQ_ERROR_UNKNOWN;
    jv_free(exit_code);
    jv error_message = jq_get_error_message(cjq_state->jq);
    if (jv_get_kind(error_message) == JV_KIND_STRING) {
      // No prefix should be added to the output of `halt_error`.
      priv_fwrite(jv_string_value(error_message), jv_string_length_bytes(jv_copy(error_message)),
          stderr, *cjq_state->dumpopts & JV_PRINT_ISATTY);
    } else if (jv_get_kind(error_message) == JV_KIND_NULL) {
      // Halt with no output
    } else if (jv_is_valid(error_message)) {
      error_message = jv_dump_string(error_message, 0);
      fprintf(stderr, "%s\n", jv_string_value(error_message));
    } // else no message on stderr; use --debug-trace to see a message
    fflush(stderr);
    jv_free(error_message);
  } else if (jv_invalid_has_msg(jv_copy(*cjq_state->result))) {
    // Uncaught jq exception
    jv msg = jv_invalid_get_msg(jv_copy(*cjq_state->result));
    jv input_pos = jq_util_input_get_position(cjq_state->jq);
    if (jv_get_kind(msg) == JV_KIND_STRING) {
      fprintf(stderr, "jq: error (at %s): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    } else {
      msg = jv_dump_string(msg, 0);
      fprintf(stderr, "jq: error (at %s) (not a string): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    }
    *cjq_state->ret = JQ_ERROR_UNKNOWN;
    jv_free(input_pos);
    jv_free(msg);
  }
  jv_free(*cjq_state->result);
}

static void jq_print(compiled_jq_state *cjq_state) {
   if ((*cjq_state->options & RAW_OUTPUT) && jv_get_kind(*cjq_state->result) == JV_KIND_STRING) {
      if (*cjq_state->options & ASCII_OUTPUT) {
        jv_dumpf(jv_copy(*cjq_state->result), stdout, JV_PRINT_ASCII);
      } else if ((*cjq_state->options & RAW_OUTPUT0) && strlen(jv_string_value(*cjq_state->result)) != (unsigned long)jv_string_length_bytes(jv_copy(*cjq_state->result))) {
        jv_free(*cjq_state->result);
        *cjq_state->result = jv_invalid_with_msg(jv_string(
              "Cannot dump a string containing NUL with --raw-output0 option"));
        jq_error(cjq_state);
      } else {
        priv_fwrite(jv_string_value(*cjq_state->result), jv_string_length_bytes(jv_copy(*cjq_state->result)),
            stdout, *cjq_state->dumpopts & JV_PRINT_ISATTY);
      }
      *cjq_state->ret = JQ_OK;
      jv_free(*cjq_state->result);
    } else {
      if (jv_get_kind(*cjq_state->result) == JV_KIND_FALSE || jv_get_kind(*cjq_state->result) == JV_KIND_NULL)
        *cjq_state->ret = JQ_OK_NULL_KIND;
      else
        *cjq_state->ret = JQ_OK;
      if (*cjq_state->options & SEQ)
        priv_fwrite("\036", 1, stdout, *cjq_state->dumpopts & JV_PRINT_ISATTY);
      jv_dump(*cjq_state->result, *cjq_state->dumpopts);  // JOHN: Where result of jq_next is printed
    }
    if (!(*cjq_state->options & RAW_NO_LF))
      priv_fwrite("\n", 1, stdout, *cjq_state->dumpopts & JV_PRINT_ISATTY);
    if (*cjq_state->options & RAW_OUTPUT0)
      priv_fwrite("\0", 1, stdout, *cjq_state->dumpopts & JV_PRINT_ISATTY);
    if (*cjq_state->options & UNBUFFERED_OUTPUT)
      fflush(stdout);
}

void _cjq_execute() {
   // DEPRECATED
   
    // Call cjq_execute
   //  cjq_execute(cjq_state.jq, cjq_state.input_state, cjq_state.jq_flags,
   //               cjq_state.dumpopts, cjq_state.options, cjq_state.ret,
   //               cjq_state.last_result, cjq_state.opcode_list, 
   //               cjq_state.opcode_list_len, 0);
}

static void _init(compiled_jq_state *cjq_state) {
   if (cjq_state->jq->halted) {
      if (cjq_state->jq->debug_trace_enabled)
        printf("\t<halted>\n");
      *cjq_state->result = jv_invalid();
      return;
    }
   *cjq_state->raising = 0;
   if (*cjq_state->backtracking) {
      // opcode = ON_BACKTRACK(opcode);   // No longer necessary :)
      *cjq_state->backtracking = 0;
      *cjq_state->raising = !jv_is_valid(cjq_state->jq->error);
    }
    cjq_state->pc++;
}

static void _do_backtrack(compiled_jq_state *cjq_state) {
   cjq_state->pc = stack_restore(cjq_state->jq);
   if (!cjq_state->pc) {
      if (!jv_is_valid(cjq_state->jq->error)) {
         jv error = cjq_state->jq->error;
         cjq_state->jq->error = jv_null();
         *cjq_state->result = error;
      }
      *cjq_state->result = jv_invalid();
   }
   *cjq_state->backtracking = 1;
}

void _opcode_TOP(void *cjq_state) { 
   _init((compiled_jq_state*)cjq_state);
 }

void _opcode_BACKTRACK_TOP(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_SUBEXP_BEGIN(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
  _init(pcjq_state);
  jv v = stack_pop(pcjq_state->jq);
  stack_push(pcjq_state->jq, jv_copy(v));
  stack_push(pcjq_state->jq, v);
  pcjq_state->jq->subexp_nest++;
  return;
}

void _opcode_BACKTRACK_SUBEXP_BEGIN(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_PUSHK_UNDER(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
  _init(pcjq_state);
  jv v = jv_array_get(jv_copy(frame_current(pcjq_state->jq)->bc->constants), *pcjq_state->pc++);
  assert(jv_is_valid(v));
  jv v2 = stack_pop(pcjq_state->jq);
  stack_push(pcjq_state->jq, v);
  stack_push(pcjq_state->jq, v2);
}

void _opcode_BACKTRACK_PUSHK_UNDER(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_INDEX(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
  _init(pcjq_state);
  jv t = stack_pop(pcjq_state->jq);
  jv k = stack_pop(pcjq_state->jq);
  // detect invalid path expression like path(reverse | .a)
  if (!path_intact(pcjq_state->jq, jv_copy(t))) {
    char keybuf[15];
    char objbuf[30];
    jv msg = jv_string_fmt(
        "Invalid path expression near attempt to access element %s of %s",
        jv_dump_string_trunc(k, keybuf, sizeof(keybuf)),
        jv_dump_string_trunc(t, objbuf, sizeof(objbuf)));
    set_error(pcjq_state->jq, jv_invalid_with_msg(msg));
    _do_backtrack(pcjq_state);
  }
  jv v = jv_get(t, jv_copy(k));
  if (jv_is_valid(v)) {
    path_append(pcjq_state->jq, k, jv_copy(v));
    stack_push(pcjq_state->jq, v);
  } else {
    jv_free(k);
    if (*pcjq_state->pc == INDEX) 
      set_error(pcjq_state->jq, v);
    else
      jv_free(v);
      _do_backtrack(pcjq_state);
  }
  return;
}

void _opcode_BACKTRACK_INDEX(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_SUBEXP_END(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
  _init(pcjq_state);
  assert(pcjq_state->jq->subexp_nest > 0);
  pcjq_state->jq->subexp_nest--;
  jv a = stack_pop(pcjq_state->jq);
  jv b = stack_pop(pcjq_state->jq);
  stack_push(pcjq_state->jq, a);
  stack_push(pcjq_state->jq, b);
  return;
}

void _opcode_BACKTRACK_SUBEXP_END(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_CALL_BUILTIN(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
  _init(pcjq_state);
  int nargs = *pcjq_state->pc++;
  jv top = stack_pop(pcjq_state->jq);
  jv *in = pcjq_state->cfunc_input;
  in[0] = top;
  for (int i = 1; i < nargs; i++) {
    in[i] = stack_pop(pcjq_state->jq);
  }
  struct cfunction* function = &frame_current(pcjq_state->jq)->bc->globals->cfunctions[*pcjq_state->pc++];
  switch (function->nargs) {
  case 1: top = ((jv (*)(jq_state *, jv))function->fptr)(pcjq_state->jq, in[0]); break;
  case 2: top = ((jv (*)(jq_state *, jv, jv))function->fptr)(pcjq_state->jq, in[0], in[1]); break;
  case 3: top = ((jv (*)(jq_state *, jv, jv, jv))function->fptr)(pcjq_state->jq, in[0], in[1], in[2]); break;
  case 4: top = ((jv (*)(jq_state *, jv, jv, jv, jv))function->fptr)(pcjq_state->jq, in[0], in[1], in[2], in[3]); break;
  case 5: top = ((jv (*)(jq_state *, jv, jv, jv, jv, jv))function->fptr)(pcjq_state->jq, in[0], in[1], in[2], in[3], in[4]); break;
  // FIXME: a) up to 7 arguments (input + 6), b) should assert
  // because the compiler should not generate this error.
  default: *pcjq_state->result = jv_invalid_with_msg(jv_string("Function takes too many arguments"));
  return;
  }

  if (jv_is_valid(top)) {
    stack_push(pcjq_state->jq, top);
  } else if (jv_invalid_has_msg(jv_copy(top))) {
    set_error(pcjq_state->jq, top);
    _do_backtrack(pcjq_state);
  } else {
    // C-coded function returns invalid w/o msg? -> backtrack, as if
    // it had returned `empty`
    _do_backtrack(pcjq_state);
  }
  return;
}

void _opcode_BACKTRACK_CALL_BUILTIN(void *cjq_state) {
  // TODO: Delete?
}

void _opcode_RET(void *cjq_state) {
  compiled_jq_state *pcjq_state = (compiled_jq_state*)cjq_state;
   _init(pcjq_state);
   jv value = stack_pop(pcjq_state->jq);
   assert(pcjq_state->jq->stk_top == frame_current(pcjq_state->jq)->retdata);
   uint16_t* retaddr = frame_current(pcjq_state->jq)->retaddr;
   if (retaddr) {
      // function return
      pcjq_state->pc = retaddr;
      frame_pop(pcjq_state->jq);
   } else {
      // top-level return, yielding value
      struct stack_pos spos = stack_get_pos(pcjq_state->jq);
      stack_push(pcjq_state->jq, jv_null());
      stack_save(pcjq_state->jq, pcjq_state->pc - 1, spos);
      pcjq_state->result = malloc(sizeof(jv));
      *pcjq_state->result = value;
      jq_print(cjq_state);    // Printing works but there's also a memory leak issue when printing
      return;
   }
   stack_push(pcjq_state->jq, value);
   return;
}

void _opcode_BACKTRACK_RET(void *cjq_state) {
   _init((compiled_jq_state*)cjq_state);
   _do_backtrack((compiled_jq_state*)cjq_state);
}