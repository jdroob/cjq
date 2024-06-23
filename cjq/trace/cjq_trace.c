#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <libgen.h>
#ifdef HAVE_SETLOCALE
#include <locale.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <processenv.h>
#include <shellapi.h>
#include <wchar.h>
#include <wtypes.h>
extern void jv_tsd_dtoa_ctx_init();
#endif

#if !defined(HAVE_ISATTY) && defined(HAVE__ISATTY)
#undef isatty
#define isatty _isatty
#endif

#if defined(HAVE_ISATTY) || defined(HAVE__ISATTY)
#define USE_ISATTY
#endif

#include "../jq/src/compile.h"
#include "../jq/src/jv.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/util.h"
#include "../jq/src/version.h"
#include "../jq/src/config_opts.inc"
#include "../jq/src/common.h"
#include "cjq_trace.h"

int jq_testsuite(jv lib_dirs, int verbose, int argc, char* argv[]);

static const char* progname;

extern uint16_t* stack_restore(jq_state *jq); // Declare global so we can use it here

/*
 * For a longer help message we could use a better option parsing
 * strategy, one that lets stack options.
 */
static void usage(int code, int keep_it_short) {
  FILE *f = stderr;

  if (code == 0)
    f = stdout;

  int ret = fprintf(f,
    "cjq - commandline JSON processor \n"
    "\nUsage:\t%s [options] <jq filter> [file...]\n"
    "\t%s [options] --args <jq filter> [strings...]\n"
    "\t%s [options] --jsonargs <jq filter> [JSON_TEXTS...]\n\n"
    "cjq is a tool for processing JSON inputs, applying the given filter to\n"
    "its JSON text inputs and producing the filter's results as JSON on\n"
    "standard output.\n\n"
    "Example:\n\n\t$ echo '{\"foo\": 0}' | ./llvmgen .\n"
    "\t{\n\t  \"foo\": 0\n\t}\n\n",
    JQ_VERSION, progname, progname, progname);
  if (keep_it_short) {
    fprintf(f,
      "For listing the command options, use %s --help.\n",
      progname);
  } else {
    (void) fprintf(f,
      "Command options:\n"
      "  -n, --null-input          use `null` as the single input value;\n"
      "  -R, --raw-input           read each line as string instead of JSON;\n"
      "  -s, --slurp               read all inputs into an array and use it as\n"
      "                            the single input value;\n"
      "  -c, --compact-output      compact instead of pretty-printed output;\n"
      "  -r, --raw-output          output strings without escapes and quotes;\n"
      "      --raw-output0         implies -r and output NUL after each output;\n"
      "  -j, --join-output         implies -r and output without newline after\n"
      "                            each output;\n"
      "  -a, --ascii-output        output strings by only ASCII characters\n"
      "                            using escape sequences;\n"
      "  -S, --sort-keys           sort keys of each object on output;\n"
      "  -C, --color-output        colorize JSON output;\n"
      "  -M, --monochrome-output   disable colored output;\n"
      "      --tab                 use tabs for indentation;\n"
      "      --indent n            use n spaces for indentation (max 7 spaces);\n"
      "      --unbuffered          flush output stream after each output;\n"
      "      --stream              parse the input value in streaming fashion;\n"
      "      --stream-errors       implies --stream and report parse error as\n"
      "                            an array;\n"
      "      --seq                 parse input/output as application/json-seq;\n"
      "  -f, --from-file file      load filter from the file;\n"
      "  -L directory              search modules from the directory;\n"
      "      --arg name value      set $name to the string value;\n"
      "      --argjson name value  set $name to the JSON value;\n"
      "      --slurpfile name file set $name to an array of JSON values read\n"
      "                            from the file;\n"
      "      --rawfile name file   set $name to string contents of file;\n"
      "      --args                consume remaining arguments as positional\n"
      "                            string values;\n"
      "      --jsonargs            consume remaining arguments as positional\n"
      "                            JSON values;\n"
      "  -e, --exit-status         set exit status code based on the output;\n"
#ifdef WIN32
      "  -b, --binary              open input/output streams in binary mode;\n"
#endif
      "  -V, --version             show the version;\n"
      "  --build-configuration     show jq's build configuration;\n"
      "  -h, --help                show the help;\n"
      "  --                        terminates argument processing;\n\n"
      "Named arguments are also available as $ARGS.named[], while\n"
      "positional arguments are available as $ARGS.positional[].\n");
  }
  exit((ret < 0 && code == 0) ? 2 : code);
}

static void die() {
  fprintf(stderr, "Use %s --help for help with command-line options,\n", progname);
  fprintf(stderr, "or see the jq manpage, or online docs  at https://jqlang.github.io/jq\n");
  exit(2);
}

static int isoptish(const char* text) {
  return text[0] == '-' && (text[1] == '-' || isalpha(text[1]));
}

static int isoption(const char* text, char shortopt, const char* longopt, size_t *short_opts) {
  if (text[0] != '-' || text[1] == '-')
    *short_opts = 0;
  if (text[0] != '-') return 0;

  // check long option
  if (text[1] == '-' && !strcmp(text+2, longopt)) return 1;
  else if (text[1] == '-') return 0;

  // must be short option; check it and...
  if (!shortopt) return 0;
  if (strchr(text, shortopt) != NULL) {
    (*short_opts)++; // ...count it (for option stacking)
    return 1;
  }
  return 0;
}

trace* init_trace() {
    trace* opcode_trace = malloc(sizeof(trace));

    opcode_trace->opcodes = malloc(sizeof(opcode_list));
    opcode_trace->opcodes->capacity = 0;
    opcode_trace->opcodes->count = 0;
    opcode_trace->opcodes->ops = NULL;
    
    opcode_trace->entries = malloc(sizeof(jq_next_entry_list));
    opcode_trace->entries->capacity = 0;
    opcode_trace->entries->count = 0;
    opcode_trace->entries->entry_locs = NULL;

    opcode_trace->inputs = malloc(sizeof(jq_next_input_list));
    opcode_trace->inputs->capacity = 0;
    opcode_trace->inputs->count = 0;
    opcode_trace->inputs->input_locs = NULL;
    
    #include <limits.h>
    opcode_trace->jq_halt_loc = UINT64_MAX;

    return opcode_trace;
}

static void flush_trace(trace* opcode_trace, PyObject* pModule_llvmlite, PyObject* pModuleLowering) {
  PyObject *pFuncSave = PyObject_GetAttrString(pModuleLowering, "save_trace");
  PyObject* opcode_trace_ptr = PyLong_FromVoidPtr((void*)opcode_trace);
  PyObject* pRes = PyObject_CallFunctionObjArgs(pFuncSave, opcode_trace_ptr, NULL);

  Py_XDECREF(pFuncSave);
  Py_XDECREF(opcode_trace_ptr);
  Py_XDECREF(pRes);
}

#define MAX_OPS 1000

trace* update_opcode_list(trace* opcode_trace, uint8_t opcode, PyObject* pModule_llvmlite, PyObject* pModuleLowering) {
  if (opcode_trace->opcodes->count >= MAX_OPS) {
      flush_trace(opcode_trace, pModule_llvmlite, pModuleLowering);
      free_trace(opcode_trace);
      opcode_trace = init_trace();
  }
  if (opcode_trace->opcodes->capacity < opcode_trace->opcodes->count + 1) {
    uint64_t oldCapacity = opcode_trace->opcodes->capacity;
    opcode_trace->opcodes->capacity = GROW_CAPACITY(oldCapacity);
    opcode_trace->opcodes->ops = GROW_ARRAY(uint8_t, opcode_trace->opcodes->ops,
    oldCapacity, opcode_trace->opcodes->capacity);
  }
  opcode_trace->opcodes->ops[opcode_trace->opcodes->count] = opcode;
  opcode_trace->opcodes->count++;

  return opcode_trace;
}

void update_entry_list(trace* opcode_trace) {
  if (opcode_trace->entries->capacity < opcode_trace->entries->count + 1) {
    uint64_t oldCapacity = opcode_trace->entries->capacity;
    opcode_trace->entries->capacity = GROW_CAPACITY(oldCapacity);
    opcode_trace->entries->entry_locs = GROW_ARRAY(uint64_t, opcode_trace->entries->entry_locs,
    oldCapacity, opcode_trace->entries->capacity);
  }
  opcode_trace->entries->entry_locs[opcode_trace->entries->count] = opcode_trace->opcodes->count;
  opcode_trace->entries->count++;
}

void update_input_list(trace* opcode_trace) {
  if (opcode_trace->inputs->capacity < opcode_trace->inputs->count + 1) {
    uint64_t oldCapacity = opcode_trace->inputs->capacity;
    opcode_trace->inputs->capacity = GROW_CAPACITY(oldCapacity);
    opcode_trace->inputs->input_locs = GROW_ARRAY(uint64_t, opcode_trace->inputs->input_locs,
    oldCapacity, opcode_trace->inputs->capacity);
  }
  opcode_trace->inputs->input_locs[opcode_trace->inputs->count] = opcode_trace->opcodes->count;
  opcode_trace->inputs->count++;
}

void update_halt_loc(trace* opcode_trace) {
  opcode_trace->jq_halt_loc = opcode_trace->opcodes->count-1; // Want jq_halt_loc to point to last opcode index
}

void free_trace(trace* opcode_trace) {
  free(opcode_trace->opcodes->ops);
  free(opcode_trace->opcodes);
  free(opcode_trace->entries->entry_locs);
  free(opcode_trace->entries);
  free(opcode_trace->inputs->input_locs);
  free(opcode_trace->inputs);
  free(opcode_trace);
  opcode_trace = NULL;
}

void* reallocate(void* pointer, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(pointer);
    return NULL;
  }
  void* result = realloc(pointer, new_size);
  if (!result) exit(EXIT_FAILURE);
  return result;
}

// DEBUGGING
static void log_write_stdout_hex(const void *ptr, size_t size, size_t nmemb) {
    const unsigned char *byte_ptr = (const unsigned char *)ptr;
    for (size_t i = 0; i < size * nmemb; i++) {
        printf("%02x ", byte_ptr[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");  // Newline for every 16 bytes
        }
    }
    printf("\n");
    fflush(stdout);
}

static void _serialize_jv(FILE* file, const jv* value);
static void serialize_jv_object(FILE* file, jvp_object* obj, int size) {
  for (int i = 0; i < size; ++i) {
    struct object_slot* slot = &obj->elements[i];
    fwrite(&slot->next, sizeof(int), 1, file);
    fwrite(&slot->hash, sizeof(uint32_t), 1, file);
    _serialize_jv(file, &slot->string); // Serialize the key (string)
    _serialize_jv(file, &slot->value);  // Serialize the value
  }
}

static void serialize_jv_array(FILE* file, jvp_array* arr, unsigned size) {
  for (int i = 0; i < size; ++i) {
      jv* p = &arr->elements[i];
      _serialize_jv(file, p); // Serialize the ith element
  }
}

static void _serialize_jv(FILE* file, const jv* value) {
  if (!value) {
    perror("invalid jv value");
    exit(EXIT_FAILURE);
  }

  jv_kind kind = jv_get_kind(*value);
  fwrite(&value->kind_flags, sizeof(value->kind_flags), 1, file);
  fwrite(&value->pad_, sizeof(value->pad_), 1, file);
  fwrite(&value->offset, sizeof(value->offset), 1, file);
  fwrite(&value->size, sizeof(value->size), 1, file);

  switch (kind) {
    case JV_KIND_NUMBER: {
      int is_lit = jv_num_is_literal(*value);
      fwrite(&is_lit, sizeof(int), 1, file);
      if (!is_lit) {
        fwrite(&value->u.number, sizeof(double), 1, file);
      } else {
        jvp_literal_number* lit = (jvp_literal_number*)value->u.ptr;
        fwrite(&lit->refcnt.count, sizeof(int), 1, file);
        fwrite(&lit->num_double, sizeof(double), 1, file);
        // Check if literal_data is NULL and write a flag
        int has_literal_data = (lit->literal_data != NULL) ? 1 : 0;
        fwrite(&has_literal_data, sizeof(int), 1, file);
        if (has_literal_data) {
          char* p = lit->literal_data;
          int len = 0;
          while (*p != '\0') { ++p; ++len; }
          fwrite(&len, sizeof(int), 1, file);
          fwrite(lit->literal_data, len + 1, 1, file);
        }
        fwrite(&lit->num_decimal.digits, sizeof(int32_t), 1, file);
        fwrite(&lit->num_decimal.exponent, sizeof(int32_t), 1, file);
        fwrite(&lit->num_decimal.bits, sizeof(uint8_t), 1, file);
        fwrite(&lit->num_decimal.lsu[0], sizeof(uint16_t), 1, file);
      }
      break;
    }
    case JV_KIND_OBJECT: {
      /**
       * See jv.c, jvp_object_new() for justificaiton of alignment
      */
      jvp_object* obj = (jvp_object*)value->u.ptr;
      fwrite(&obj->refcnt.count, sizeof(int), 1, file);
      serialize_jv_object(file, obj, value->size);
      fwrite(&obj->next_free, sizeof(int), 1, file);
      int* hashbuckets = (int*)(&obj->elements[value->size]);
      for (int i=0; i<value->size*2; ++i) {
        fwrite(&hashbuckets[i], sizeof(int), 1, file);
      }
      break;
    }
    case JV_KIND_ARRAY: {
      jvp_array* arr = (jvp_array*)value->u.ptr;
      fwrite(&arr->alloc_length, sizeof(int), 1, file);   // So deserialize() can determine how much memory to allocate
      fwrite(&arr->refcnt.count, sizeof(int), 1, file);
      fwrite(&arr->length, sizeof(int), 1, file);
      fwrite(&arr->alloc_length, sizeof(arr->alloc_length), 1, file);
      serialize_jv_array(file, arr, arr->length);
      break;
    }
    case JV_KIND_STRING: {
      jvp_string* str = (jvp_string*)value->u.ptr;
      fwrite(&str->alloc_length, sizeof(int), 1, file);   // So deserialize() can determine how much memory to allocate
      fwrite(&str->refcnt.count, sizeof(int), 1, file);
      fwrite(&str->hash, sizeof(uint32_t), 1, file);
      fwrite(&str->alloc_length, sizeof(uint32_t), 1, file);
      fwrite(&str->length_hashed, sizeof(uint32_t), 1, file);
      size_t len = str->alloc_length;  
      fwrite(&str->data, len+1, 1, file);
      break;
    }
    case JV_KIND_INVALID: {
      jvp_invalid* inv = (jvp_invalid*)value->u.ptr;
      
      if (inv == NULL) {
          int is_null = 1;
          fwrite(&is_null, sizeof(int), 1, file);;
      } else {
          int is_null = 0;
          fwrite(&is_null, sizeof(int), 1, file);
          fwrite(&inv->refcnt.count, sizeof(int), 1, file);
          _serialize_jv(file, &inv->errmsg);
      }

      break;
    }
    default: {    /* JV_KIND_TRUE, JV_KIND_FALSE, JV_KIND_NULL */ 
      size_t total_size = sizeof(double);
      if (fwrite(&value->u, total_size, 1, file) != 1) {
          perror("Failed to write jv string to file");
      }
      break;
    }
  }
}

static void serialize_jv(const char* filename, jv* value) {
  FILE* file = fopen(filename, "wb");
  if (!file) {
      perror("Failed to open file for writing");
      return;
  }
  _serialize_jv(file, value);
  fclose(file);
}

static void _serialize_sym_table(FILE* file, struct symbol_table* table) {
  fwrite(&table->ncfunctions, sizeof(int), 1, file);

  _serialize_jv(file, &table->cfunc_names);

  for (int i=0; i<table->ncfunctions; ++i) {
    int len = 0;
    char *p = (char*)table->cfunctions[i].name;
    while(*p != '\0') { ++len; p++; }
    fwrite(&len, sizeof(int), 1, file);

    // TODO: Probably don't need a loop for this
    for (int j=0; j<len+1; ++j) {
      fwrite(&table->cfunctions[i].name[j], sizeof(char), 1, file);
    }

    fwrite(&table->cfunctions[i].nargs, sizeof(int), 1, file);
  }
}

static void serialize_sym_table(const char* filename, struct symbol_table* table) {
  FILE* file = fopen(filename, "wb");
  if (!file) {
      perror("Failed to open file for writing");
      return;
  }
  _serialize_sym_table(file, table);
  fclose(file);
}

#define MAX_SERIALIZED_BCS 1000 // Adjust this as needed

static struct bytecode* serialized_bcs[MAX_SERIALIZED_BCS];
static int num_serialized_bcs = 0;

// To prevent infinite recursion (e.g. serialize subfunc S, serialize parent P, serialize subfunc S, ...)
static int is_bc_serialized(const struct bytecode* bc) {
    for (int i = 0; i < num_serialized_bcs; ++i) {
        if (serialized_bcs[i] == bc) {
            return 1;
        }
    }
    return 0;
}

static int get_bc_idx(const struct bytecode* bc) {
    for (int i = 0; i < num_serialized_bcs; ++i) {
        if (serialized_bcs[i] == bc) {
            return i;
        }
    }
    return -1;
}

static void add_serialized_bc(const struct bytecode* bc) {
    if (num_serialized_bcs < MAX_SERIALIZED_BCS) {
        serialized_bcs[num_serialized_bcs++] = (struct bytecode*)bc;
    }
}

static void reset_serialized_bcs() {
    num_serialized_bcs = 0;
}

static void _serialize_bc(FILE* file, const struct bytecode* bc);
static void serialize_bc_parent(FILE* file, const struct bytecode* ptr) {
  if (ptr) {
    if (is_bc_serialized(ptr)) {
      uint32_t parent_flag = 0xFFFFFFFD;
      fwrite(&parent_flag, sizeof(uint32_t), 1, file);
      int idx = get_bc_idx(ptr);
      if (idx == -1) {
        printf("Error: could not find parent idx\n");
        exit(EXIT_FAILURE);
      }
      fwrite(&idx, sizeof(int), 1, file);
    } else {
      // TODO: Should we ever get here?
      _serialize_bc(file, ptr);
    }
  } else {
    // If ptr is NULL, write a sentinel value to indicate null pointer
    uint32_t null_value = 0xFFFFFFFF;
    fwrite(&null_value, sizeof(uint32_t), 1, file);
  }
}

static void _serialize_bc(FILE* file, const struct bytecode* bc) {
  if (!bc) {
    // Serialize a sentinel value to indicate a null pointer
    uint32_t null_value = 0xFFFFFFFF;
    fwrite(&null_value, sizeof(uint32_t), 1, file);
    return;
  }

  // This prevents stack overflow due to infinite recursion
  if (is_bc_serialized(bc)) {
      uint32_t duplicate_ptr = 0xFFFFFFFE;
      fwrite(&duplicate_ptr, sizeof(uint32_t), 1, file);
      return;
  }

  // Mark this bytecode struct as serialized
  add_serialized_bc(bc);

  fwrite(&bc->codelen, sizeof(int), 1, file);   // Serialize first so deserialize_bc knows how much memory to alloc
  fwrite(bc->code, sizeof(uint16_t), bc->codelen, file);
  fwrite(&bc->nlocals, sizeof(int), 1, file);
  fwrite(&bc->nclosures, sizeof(int), 1, file);
  _serialize_jv(file, &bc->constants);
  
  // Serialize the parent pointer
  serialize_bc_parent(file, bc->parent);

  if (!bc->parent)
    _serialize_sym_table(file, bc->globals);

  // Serialize subfunctions recursively
  fwrite(&bc->nsubfunctions, sizeof(int), 1, file);
  for (int i = 0; i < bc->nsubfunctions; ++i) {
      _serialize_bc(file, bc->subfunctions[i]);
  }
  _serialize_jv(file, &bc->debuginfo);
}

static void serialize_bc(const char* filename, const struct bytecode* bc) {
  FILE* file = fopen(filename, "wb");
  if (!file) {
      perror("Failed to open file for writing");
      return;
  }
  _serialize_bc(file, bc);
  fclose(file);
}

static int process(jq_state* jq, jv value, int flags, int dumpopts, int options, trace** opcode_trace, PyObject* pModule_llvmlite, PyObject* pModuleLowering) {
  int ret = JQ_OK_NO_OUTPUT; // No valid results && -e -> exit(4)
  jq_start(jq, value, flags);   // John: Pushes entire value (all json objects) to stack
  jv result;
  while (jv_is_valid(result = jq_next(jq, (void**)opcode_trace, pModule_llvmlite, pModuleLowering))) {
    if ((options & RAW_OUTPUT) && jv_get_kind(result) == JV_KIND_STRING) {
      if (options & ASCII_OUTPUT) {
        jv_dumpf(jv_copy(result), stdout, JV_PRINT_ASCII);
      } else if ((options & RAW_OUTPUT0) && strlen(jv_string_value(result)) != (unsigned long)jv_string_length_bytes(jv_copy(result))) {
        jv_free(result);
        result = jv_invalid_with_msg(jv_string(
              "Cannot dump a string containing NUL with --raw-output0 option"));
        break;
      } else {
        priv_fwrite(jv_string_value(result), jv_string_length_bytes(jv_copy(result)),
            stdout, dumpopts & JV_PRINT_ISATTY);
      }
      ret = JQ_OK;
      jv_free(result);
    } else {
      if (jv_get_kind(result) == JV_KIND_FALSE || jv_get_kind(result) == JV_KIND_NULL)
        ret = JQ_OK_NULL_KIND;
      else
        ret = JQ_OK;
      if (options & SEQ)
        priv_fwrite("\036", 1, stdout, dumpopts & JV_PRINT_ISATTY);
      jv_dump(result, dumpopts);  // JOHN: Where result of jq_next is printed
    }
    if (!(options & RAW_NO_LF))
      priv_fwrite("\n", 1, stdout, dumpopts & JV_PRINT_ISATTY);
    if (options & RAW_OUTPUT0)
      priv_fwrite("\0", 1, stdout, dumpopts & JV_PRINT_ISATTY);
    if (options & UNBUFFERED_OUTPUT)
      fflush(stdout);
  }
  if (jq_halted(jq)) {
    update_halt_loc(*opcode_trace);
    // jq program invoked `halt` or `halt_error`
    jv exit_code = jq_get_exit_code(jq);
    if (!jv_is_valid(exit_code))
      ret = JQ_OK;
    else if (jv_get_kind(exit_code) == JV_KIND_NUMBER)
      ret = jv_number_value(exit_code);
    else
      ret = JQ_ERROR_UNKNOWN;
    jv_free(exit_code);
    jv error_message = jq_get_error_message(jq);
    if (jv_get_kind(error_message) == JV_KIND_STRING) {
      // No prefix should be added to the output of `halt_error`.
      priv_fwrite(jv_string_value(error_message), jv_string_length_bytes(jv_copy(error_message)),
          stderr, dumpopts & JV_PRINT_ISATTY);
    } else if (jv_get_kind(error_message) == JV_KIND_NULL) {
      // Halt with no output
    } else if (jv_is_valid(error_message)) {
      error_message = jv_dump_string(error_message, 0);
      fprintf(stderr, "%s\n", jv_string_value(error_message));
    } // else no message on stderr; use --debug-trace to see a message
    fflush(stderr);
    jv_free(error_message);
  } else if (jv_invalid_has_msg(jv_copy(result))) {
    // Uncaught jq exception
    jv msg = jv_invalid_get_msg(jv_copy(result));
    jv input_pos = jq_util_input_get_position(jq);
    if (jv_get_kind(msg) == JV_KIND_STRING) {
      fprintf(stderr, "jq: error (at %s): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    } else {
      msg = jv_dump_string(msg, 0);
      fprintf(stderr, "jq: error (at %s) (not a string): %s\n",
              jv_string_value(input_pos), jv_string_value(msg));
    }
    ret = JQ_ERROR_UNKNOWN;
    jv_free(input_pos);
    jv_free(msg);
  }
  jv_free(result);
  return ret;
}

static void debug_cb(void *data, jv input) {
  int dumpopts = *(int*)data;
  jv_dumpf(JV_ARRAY(jv_string("DEBUG:"), input), stderr, dumpopts & ~(JV_PRINT_PRETTY));
  fprintf(stderr, "\n");
}

static void stderr_cb(void *data, jv input) {
  if (jv_get_kind(input) == JV_KIND_STRING) {
    int dumpopts = *(int *)data;
    priv_fwrite(jv_string_value(input), jv_string_length_bytes(jv_copy(input)),
        stderr, dumpopts & JV_PRINT_ISATTY);
  } else {
    input = jv_dump_string(input, 0);
    fprintf(stderr, "%s", jv_string_value(input));
  }
  jv_free(input);
}

#ifdef WIN32
int umain(int argc, char* argv[]);

int wmain(int argc, wchar_t* wargv[]) {
  size_t arg_sz;
  char **argv = alloca(argc * sizeof(wchar_t*));
  for (int i = 0; i < argc; i++) {
    argv[i] = alloca((arg_sz = WideCharToMultiByte(CP_UTF8,
                                                   0,
                                                   wargv[i],
                                                   -1, 0, 0, 0, 0)));
    WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, argv[i], arg_sz, 0, 0);
  }
  return umain(argc, argv);
}

int umain(int argc, char* argv[]) {
#else /*}*/
int cjq_trace(int argc, char* argv[], trace* opcode_trace, PyObject* pModule_llvmlite, PyObject* pModuleLowering) {
#endif
  jq_state* jq = NULL;
  jq_util_input_state* input_state = NULL;
  int ret = JQ_OK_NO_OUTPUT;
  int compiled = 0;
  int parser_flags = 0;
  int nfiles = 0;
  int last_result = -1; /* -1 = no result, 0=null or false, 1=true */
  int badwrite;
  int options = 0;

#ifdef HAVE_SETLOCALE
  (void) setlocale(LC_ALL, "");
#endif

#ifdef __OpenBSD__
  if (pledge("stdio rpath", NULL) == -1) {
    perror("pledge");
    exit(JQ_ERROR_SYSTEM);
  }
#endif

#ifdef WIN32
  jv_tsd_dtoa_ctx_init();
  fflush(stdout);
  fflush(stderr);
  _setmode(fileno(stdout), _O_TEXT | _O_U8TEXT);
  _setmode(fileno(stderr), _O_TEXT | _O_U8TEXT);
#endif

  jv ARGS = jv_array(); /* positional arguments */
  jv program_arguments = jv_object(); /* named arguments */

  if (argc) progname = argv[0];

  jq = jq_init();
  if (jq == NULL) {
    perror("jq_init");
    ret = JQ_ERROR_SYSTEM;
    goto out;
  }

  int dumpopts = JV_PRINT_INDENT_FLAGS(2);
  const char* program = 0;

  input_state = jq_util_input_init(NULL, NULL); // XXX add err_cb

  int further_args_are_strings = 0;
  int further_args_are_json = 0;
  int args_done = 0;
  int jq_flags = 0;
  size_t short_opts = 0;
  jv lib_search_paths = jv_null();
  for (int i=1; i<argc; i++, short_opts = 0) {
    if (args_done || !isoptish(argv[i])) {
      if (!program) {
        program = argv[i];
      } else if (further_args_are_strings) {
        ARGS = jv_array_append(ARGS, jv_string(argv[i]));
      } else if (further_args_are_json) {
        jv v =  jv_parse(argv[i]);
        if (!jv_is_valid(v)) {
          fprintf(stderr, "%s: invalid JSON text passed to --jsonargs\n", progname);
          die();
        }
        ARGS = jv_array_append(ARGS, v);
      } else {      // this is where the json files are read?
        jq_util_input_add_input(input_state, argv[i]);
        nfiles++;
      }
    } else if (!strcmp(argv[i], "--")) {
      args_done = 1;
    } else {
      if (argv[i][1] == 'L') {
        if (jv_get_kind(lib_search_paths) == JV_KIND_NULL)
          lib_search_paths = jv_array();
        if (argv[i][2] != 0) { // -Lname (faster check than strlen)
            lib_search_paths = jv_array_append(lib_search_paths, jq_realpath(jv_string(argv[i]+2)));
        } else if (i >= argc - 1) {
          fprintf(stderr, "-L takes a parameter: (e.g. -L /search/path or -L/search/path)\n");
          die();
        } else {
          lib_search_paths = jv_array_append(lib_search_paths, jq_realpath(jv_string(argv[i+1])));
          i++;
        }
        continue;
      }

      if (isoption(argv[i], 's', "slurp", &short_opts)) {
        options |= SLURP;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'r', "raw-output", &short_opts)) {
        options |= RAW_OUTPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 0, "raw-output0", &short_opts)) {
        options |= RAW_OUTPUT | RAW_NO_LF | RAW_OUTPUT0;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'j', "join-output", &short_opts)) {
        options |= RAW_OUTPUT | RAW_NO_LF;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'c', "compact-output", &short_opts)) {
        dumpopts &= ~(JV_PRINT_TAB | JV_PRINT_INDENT_FLAGS(7));
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'C', "color-output", &short_opts)) {
        options |= COLOR_OUTPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'M', "monochrome-output", &short_opts)) {
        options |= NO_COLOR_OUTPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'a', "ascii-output", &short_opts)) {
        options |= ASCII_OUTPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 0, "unbuffered", &short_opts)) {
        options |= UNBUFFERED_OUTPUT;
        continue;
      }
      if (isoption(argv[i], 'S', "sort-keys", &short_opts)) {
        options |= SORTED_OUTPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'R', "raw-input", &short_opts)) {
        options |= RAW_INPUT;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'n', "null-input", &short_opts)) {
        options |= PROVIDE_NULL;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'f', "from-file", &short_opts)) {
        options |= FROM_FILE;
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'b', "binary", &short_opts)) {
#ifdef WIN32
        fflush(stdout);
        fflush(stderr);
        _setmode(fileno(stdin),  _O_BINARY);
        _setmode(fileno(stdout), _O_BINARY);
        _setmode(fileno(stderr), _O_BINARY);
#endif
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 0, "tab", &short_opts)) {
        dumpopts &= ~JV_PRINT_INDENT_FLAGS(7);
        dumpopts |= JV_PRINT_TAB | JV_PRINT_PRETTY;
        continue;
      }
      if (isoption(argv[i], 0, "indent", &short_opts)) {
        if (i >= argc - 1) {
          fprintf(stderr, "%s: --indent takes one parameter\n", progname);
          die();
        }
        dumpopts &= ~(JV_PRINT_TAB | JV_PRINT_INDENT_FLAGS(7));
        int indent = atoi(argv[i+1]);
        if (indent < -1 || indent > 7) {
          fprintf(stderr, "%s: --indent takes a number between -1 and 7\n", progname);
          die();
        }
        dumpopts |= JV_PRINT_INDENT_FLAGS(indent);
        i++;
        continue;
      }
      if (isoption(argv[i], 0, "seq", &short_opts)) {
        options |= SEQ;
        continue;
      }
      if (isoption(argv[i], 0, "stream", &short_opts)) {
        parser_flags |= JV_PARSE_STREAMING;
        continue;
      }
      if (isoption(argv[i], 0, "stream-errors", &short_opts)) {
        parser_flags |= JV_PARSE_STREAMING | JV_PARSE_STREAM_ERRORS;
        continue;
      }
      if (isoption(argv[i], 'e', "exit-status", &short_opts)) {
        options |= EXIT_STATUS;
        if (!short_opts) continue;
      }
      // FIXME: For --arg* we should check that the varname is acceptable
      if (isoption(argv[i], 0, "args", &short_opts)) {
        further_args_are_strings = 1;
        further_args_are_json = 0;
        continue;
      }
      if (isoption(argv[i], 0, "jsonargs", &short_opts)) {
        further_args_are_strings = 0;
        further_args_are_json = 1;
        continue;
      }
      if (isoption(argv[i], 0, "arg", &short_opts)) {
        if (i >= argc - 2) {
          fprintf(stderr, "%s: --arg takes two parameters (e.g. --arg varname value)\n", progname);
          die();
        }
        if (!jv_object_has(jv_copy(program_arguments), jv_string(argv[i+1])))
          program_arguments = jv_object_set(program_arguments, jv_string(argv[i+1]), jv_string(argv[i+2]));
        i += 2; // skip the next two arguments
        continue;
      }
      if (isoption(argv[i], 0, "argjson", &short_opts)) {
        if (i >= argc - 2) {
          fprintf(stderr, "%s: --argjson takes two parameters (e.g. --argjson varname text)\n", progname);
          die();
        }
        if (!jv_object_has(jv_copy(program_arguments), jv_string(argv[i+1]))) {
          jv v = jv_parse(argv[i+2]);
          if (!jv_is_valid(v)) {
            fprintf(stderr, "%s: invalid JSON text passed to --argjson\n", progname);
            die();
          }
          program_arguments = jv_object_set(program_arguments, jv_string(argv[i+1]), v);
        }
        i += 2; // skip the next two arguments
        continue;
      }
      if (isoption(argv[i], 0, "rawfile", &short_opts) ||
          isoption(argv[i], 0, "slurpfile", &short_opts)) {
        int raw = isoption(argv[i], 0, "rawfile", &short_opts);
        const char *which;
        if (raw)
          which = "rawfile";
        else
          which = "slurpfile";
        if (i >= argc - 2) {
          fprintf(stderr, "%s: --%s takes two parameters (e.g. --%s varname filename)\n", progname, which, which);
          die();
        }
        if (!jv_object_has(jv_copy(program_arguments), jv_string(argv[i+1]))) {
          jv data = jv_load_file(argv[i+2], raw);
          if (!jv_is_valid(data)) {
            data = jv_invalid_get_msg(data);
            fprintf(stderr, "%s: Bad JSON in --%s %s %s: %s\n", progname, which,
                    argv[i+1], argv[i+2], jv_string_value(data));
            jv_free(data);
            ret = JQ_ERROR_SYSTEM;
            goto out;
          }
          program_arguments = jv_object_set(program_arguments, jv_string(argv[i+1]), data);
        }
        i += 2; // skip the next two arguments
        continue;
      }
      if (isoption(argv[i],  0,  "debug-dump-disasm", &short_opts)) {
        options |= DUMP_DISASM;
        continue;
      }
      if (isoption(argv[i],  0,  "debug-trace=all", &short_opts)) {
        jq_flags |= JQ_DEBUG_TRACE_ALL;
        if (!short_opts) continue;
      }
      if (isoption(argv[i],  0,  "debug-trace", &short_opts)) {
        jq_flags |= JQ_DEBUG_TRACE;
        continue;
      }
      if (isoption(argv[i], 'h', "help", &short_opts)) {
        usage(0, 0);
        if (!short_opts) continue;
      }
      if (isoption(argv[i], 'V', "version", &short_opts)) {
        printf("jq-%s\n", JQ_VERSION);
        ret = JQ_OK;
        goto out;
      }
      if (isoption(argv[i], 0, "build-configuration", &short_opts)) {
        printf("%s\n", JQ_CONFIG);
        ret = JQ_OK;
        goto out;
      }
      if (isoption(argv[i], 0, "run-tests", &short_opts)) {
        i++;
        // XXX Pass program_arguments, even a whole jq_state *, through;
        // could be useful for testing
        ret = jq_testsuite(lib_search_paths,
                           (options & DUMP_DISASM) || (jq_flags & JQ_DEBUG_TRACE),
                           argc - i, argv + i);
        goto out;
      }

      // check for unknown options... if this argument was a short option
      if (strlen(argv[i]) != short_opts + 1) {
        fprintf(stderr, "%s: Unknown option %s\n", progname, argv[i]);
        die();
      }
    }
  }

#ifdef USE_ISATTY
  if (isatty(STDOUT_FILENO)) {
#ifndef WIN32
    dumpopts |= JV_PRINT_ISATTY | JV_PRINT_COLOR;
#else
  /* Verify we actually have the console, as the NUL device is also regarded as
     tty.  Windows can handle color if ANSICON (or ConEmu) is installed, or
     Windows 10 supports the virtual terminal */
    DWORD mode;
    HANDLE con = GetStdHandle(STD_OUTPUT_HANDLE);
    if (GetConsoleMode(con, &mode)) {
      dumpopts |= JV_PRINT_ISATTY;
      if (getenv("ANSICON") != NULL ||
          SetConsoleMode(con, mode | 4/*ENABLE_VIRTUAL_TERMINAL_PROCESSING*/))
        dumpopts |= JV_PRINT_COLOR;
    }
#endif
    if (dumpopts & JV_PRINT_COLOR) {
      char *no_color = getenv("NO_COLOR");
      if (no_color != NULL && no_color[0] != '\0')
        dumpopts &= ~JV_PRINT_COLOR;
    }
  }
#endif
  if (options & SORTED_OUTPUT) dumpopts |= JV_PRINT_SORTED;
  if (options & ASCII_OUTPUT) dumpopts |= JV_PRINT_ASCII;
  if (options & COLOR_OUTPUT) dumpopts |= JV_PRINT_COLOR;
  if (options & NO_COLOR_OUTPUT) dumpopts &= ~JV_PRINT_COLOR;

  if (getenv("JQ_COLORS") != NULL && !jq_set_colors(getenv("JQ_COLORS")))
      fprintf(stderr, "Failed to set $JQ_COLORS\n");

  if (jv_get_kind(lib_search_paths) == JV_KIND_NULL) {
    // Default search path list
    lib_search_paths = JV_ARRAY(jv_string("~/.jq"),
                                jv_string("$ORIGIN/../lib/jq"),
                                jv_string("$ORIGIN/../lib"));
  }
  jq_set_attr(jq, jv_string("JQ_LIBRARY_PATH"), lib_search_paths);

  char *origin = strdup(argv[0]);
  if (origin == NULL) {
    fprintf(stderr, "jq: error: out of memory\n");
    exit(1);
  }
  jq_set_attr(jq, jv_string("JQ_ORIGIN"), jv_string(dirname(origin)));
  free(origin);

  if (strchr(JQ_VERSION, '-') == NULL)
    jq_set_attr(jq, jv_string("VERSION_DIR"), jv_string(JQ_VERSION));
  else
    jq_set_attr(jq, jv_string("VERSION_DIR"), jv_string_fmt("%.*s-master", (int)(strchr(JQ_VERSION, '-') - JQ_VERSION), JQ_VERSION));

#ifdef USE_ISATTY
  if (!program && (!isatty(STDOUT_FILENO) || !isatty(STDIN_FILENO)))
    program = ".";
#endif

  if (!program) usage(2, 1);

  if (options & FROM_FILE) {
    char *program_origin = strdup(program);
    if (program_origin == NULL) {
      perror("malloc");
      exit(2);
    }

    jv data = jv_load_file(program, 1);   // JOHN: This loads jq program from *.jq file
    if (!jv_is_valid(data)) {
      data = jv_invalid_get_msg(data);
      fprintf(stderr, "%s: %s\n", progname, jv_string_value(data));
      jv_free(data);
      ret = JQ_ERROR_SYSTEM;
      goto out;
    }
    jq_set_attr(jq, jv_string("PROGRAM_ORIGIN"), jq_realpath(jv_string(dirname(program_origin))));
    ARGS = JV_OBJECT(jv_string("positional"), ARGS,
                     jv_string("named"), jv_copy(program_arguments));
    program_arguments = jv_object_set(program_arguments, jv_string("ARGS"), jv_copy(ARGS));
    if (!jv_object_has(jv_copy(program_arguments), jv_string("JQ_BUILD_CONFIGURATION")))
      program_arguments = jv_object_set(program_arguments,
                                        jv_string("JQ_BUILD_CONFIGURATION"),
                                        jv_string(JQ_CONFIG)); /* named arguments */
    compiled = jq_compile_args(jq, jv_string_value(data), jv_copy(program_arguments));
    free(program_origin);
    jv_free(data);
  } else {
    jq_set_attr(jq, jv_string("PROGRAM_ORIGIN"), jq_realpath(jv_string("."))); // XXX is this good?
    ARGS = JV_OBJECT(jv_string("positional"), ARGS,
                     jv_string("named"), jv_copy(program_arguments));
    program_arguments = jv_object_set(program_arguments, jv_string("ARGS"), jv_copy(ARGS));
    if (!jv_object_has(jv_copy(program_arguments), jv_string("JQ_BUILD_CONFIGURATION")))
      program_arguments = jv_object_set(program_arguments,
                                        jv_string("JQ_BUILD_CONFIGURATION"),
                                        jv_string(JQ_CONFIG)); /* named arguments */
    compiled = jq_compile_args(jq, program, jv_copy(program_arguments));
  }

  reset_serialized_bcs();
  serialize_bc("serialize.bin", jq->bc);

  if (!compiled){
    ret = JQ_ERROR_COMPILE;
    goto out;
  }
  
  if (options & DUMP_DISASM) {
    jq_dump_disassembly(jq, 0);
    printf("\n");  
  }

  if ((options & SEQ))
    parser_flags |= JV_PARSE_SEQ;

  if ((options & RAW_INPUT)) {
    jq_util_input_set_parser(input_state, NULL, (options & SLURP) ? 1 : 0);
  }
  else {
    jq_util_input_set_parser(input_state, jv_parser_new(parser_flags), (options & SLURP) ? 1 : 0);
  }

  // Let jq program read from inputs
  jq_set_input_cb(jq, jq_util_input_next_input_cb, input_state);

  // Let jq program call `debug` builtin and have that go somewhere
  jq_set_debug_cb(jq, debug_cb, &dumpopts);

  // Let jq program call `stderr` builtin and have that go somewhere
  jq_set_stderr_cb(jq, stderr_cb, &dumpopts);

  if (nfiles == 0) {
    jq_util_input_add_input(input_state, "-");
  }

  // Tracing run
  if (options & PROVIDE_NULL) {
    jv njv = jv_null();
    update_input_list(opcode_trace);
    ret = process(jq, jv_null(), jq_flags, dumpopts, options, &opcode_trace, pModule_llvmlite, pModuleLowering);
  } else {
    jv value;
    while (jq_util_input_errors(input_state) == 0 &&
           (jv_is_valid((value = jq_util_input_next_input(input_state))) || jv_invalid_has_msg(jv_copy(value)))) {
      if (jv_is_valid(value)) {
        update_input_list(opcode_trace);
        ret = process(jq, value, jq_flags, dumpopts, options, &opcode_trace, pModule_llvmlite, pModuleLowering);
        if (ret <= 0 && ret != JQ_OK_NO_OUTPUT)
          last_result = (ret != JQ_OK_NULL_KIND);
        if (jq_halted(jq))
          break;
        continue;
      }

      // Parse error
      jv msg = jv_invalid_get_msg(value);
      if (!(options & SEQ)) {
        ret = JQ_ERROR_UNKNOWN;
        fprintf(stderr, "jq: parse error: %s\n", jv_string_value(msg));
        jv_free(msg);
        break;
      }
      // --seq -> errors are not fatal
      fprintf(stderr, "jq: ignoring parse error: %s\n", jv_string_value(msg));
      jv_free(msg);
    }
  }

  if (jq_util_input_errors(input_state) != 0)
    ret = JQ_ERROR_SYSTEM;
 
out:
  badwrite = ferror(stdout);
// JOHN: Commented below code out because it was preventing further printing
//   if (fclose(stdout)!=0 || badwrite) {   // JOHN: Specifically, fclose(stdout)
//     fprintf(stderr,"jq: error: writing output failed: %s\n", strerror(errno));
//     ret = JQ_ERROR_SYSTEM;
//   }

  flush_trace(opcode_trace, pModule_llvmlite, pModuleLowering);
  free_trace(opcode_trace);
  // opcode_trace = init_trace();

  jv_free(ARGS);
  jv_free(program_arguments);
  jq_util_input_free(&input_state);
  jq_teardown(&jq);

  if (options & EXIT_STATUS) {
    if (ret != JQ_OK_NO_OUTPUT)
      jq_exit_with_status(ret);
    else
      switch (last_result) {
        case -1: jq_exit_with_status(JQ_OK_NO_OUTPUT);
        case  0: jq_exit_with_status(JQ_OK_NULL_KIND);
        default: jq_exit_with_status(JQ_OK);
      }
  } else
    return 0;
}