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
#include "../jq/src/common.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/util.h"
#include "../jq/src/version.h"
#include "../jq/src/builtin.h"
#include "../jq/src/config_opts.inc"
#include "cjq_bootstrap.h"

int jq_testsuite(jv lib_dirs, int verbose, int argc, char* argv[]);

static const char* progname;

/*
 * For a longer help message we could use a better option parsing
 * strategy, one that lets stack options.
 */
static void usage(int code, int keep_it_short) {
  FILE *f = stderr;

  if (code == 0)
    f = stdout;

  int ret = fprintf(f,
    "jq - commandline JSON processor [version %s]\n"
    "\nUsage:\t%s [options] <jq filter> [file...]\n"
    "\t%s [options] --args <jq filter> [strings...]\n"
    "\t%s [options] --jsonargs <jq filter> [JSON_TEXTS...]\n\n"
    "jq is a tool for processing JSON inputs, applying the given filter to\n"
    "its JSON text inputs and producing the filter's results as JSON on\n"
    "standard output.\n\n"
    "The simplest filter is ., which copies jq's input to its output\n"
    "unmodified except for formatting. For more advanced filters see\n"
    "the jq(1) manpage (\"man jq\") and/or https://jqlang.github.io/jq/.\n\n"
    "Example:\n\n\t$ echo '{\"foo\": 0}' | jq .\n"
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

void cjq_init(cjq_state* cjq, int ret, int jq_flags, int options, int dumpopts, int last_result,
 jq_util_input_state* input_state, jq_state* jq) {
  int* pret = malloc(sizeof(int)); *pret = ret;
  int* pjq_flags = malloc(sizeof(int)); *pjq_flags = jq_flags;
  int* poptions = malloc(sizeof(int)); *poptions = options;
  int* pdumpopts = malloc(sizeof(int)); *pdumpopts = dumpopts;
  int* plast_result = malloc(sizeof(int)); *plast_result = last_result;   // TODO: I don't think last_result needs to be passed in here
  int* pbacktracking = malloc(sizeof(int)); *pbacktracking = 0;
  int* praising = malloc(sizeof(int)); *praising = 0;
  uint8_t* pfallthrough = malloc(sizeof(uint8_t)); *pfallthrough = 0;
  uint16_t* popcode = malloc(sizeof(uint16_t)); *popcode = -1;
  cjq->ret = pret; pret = NULL;
  cjq->jq_flags = pjq_flags; pjq_flags = NULL;
  cjq->options = poptions; poptions = NULL;
  cjq->dumpopts = pdumpopts; pdumpopts = NULL;
  cjq->last_result = plast_result; plast_result = NULL;
  cjq->fallthrough = pfallthrough; pfallthrough = NULL;
  cjq->pc = NULL;
  cjq->opcode = popcode; popcode = NULL;
  cjq->jq = jq; jq = NULL;
  cjq->result = malloc(sizeof(jv));
  cjq->backtracking = pbacktracking; pbacktracking = NULL;
  cjq->raising = praising; praising = NULL;
  cjq->input_state = input_state; input_state = NULL;
  cjq->value = NULL;
}

void cjq_free(cjq_state* cjq) {
  cjq->pc = NULL;
  free(cjq->opcode); cjq->opcode = NULL;
  free(cjq->ret); cjq->ret = NULL;
  free(cjq->jq_flags); cjq->jq_flags = NULL;
  free(cjq->options); cjq->options = NULL;
  free(cjq->dumpopts); cjq->dumpopts = NULL;
  free(cjq->last_result); cjq->last_result = NULL;
  if (cjq->value) {
    if (!cjq->value->u.ptr) 
      jv_free(*cjq->value);
    free(cjq->value);
    cjq->value = NULL;
  }
  free(cjq->result); cjq->result = NULL;
  free(cjq->raising); cjq->raising = NULL;
  free(cjq->backtracking); cjq->backtracking = NULL;
  free(cjq->fallthrough); cjq->fallthrough = NULL;
  free(cjq); cjq = NULL;
}

void free_cfunction_names(void* bc) {
  struct bytecode* pbc = (struct bytecode*)bc;
  for (int i=0; i<pbc->globals->ncfunctions; ++i) {
    free((void*)pbc->globals->cfunctions[i].name);
  }
}

cjq_state* cjq_mem_alloc() {
  cjq_state* p = malloc(sizeof(cjq_state));
  if (!p) {
    fprintf(stderr, "Failed to allocate memory for cjq_state object.\n");
    return NULL;
  }

  // Initialize member pointers to NULL
  p->ret = NULL;
  p->jq_flags = NULL;
  p->dumpopts = NULL;
  p->options = NULL;
  p->last_result = NULL;
  p->raising = NULL;
  p->input_state = NULL;
  p->value = NULL;
  p->result = NULL;
  p->jq = NULL;
  p->pc = NULL;
  p->opcode = NULL;
  p->backtracking = NULL;
  p->fallthrough = NULL;

  return p;
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

static void _deserialize_jv(FILE* file, jv* value);

static void deserialize_jv_object(FILE* file, jvp_object* obj, int size) {
  for (int i = 0; i<size; ++i) {
      struct object_slot* slot = &obj->elements[i];
      fread(&slot->next, sizeof(int), 1, file);
      fread(&slot->hash, sizeof(uint32_t), 1, file);
      _deserialize_jv(file, &slot->string); // Deserialize the key (string)
      _deserialize_jv(file, &slot->value);  // Deserialize the value
  }
}

static void deserialize_jv_array(FILE *file, int size, jvp_array* arr) {
  for (int i = 0; i<size; ++i) {
      _deserialize_jv(file, &arr->elements[i]); // Deserialize the ith element
  }
}

static void _deserialize_jv(FILE* file, jv* value) {
  if (!value) {
      perror("Failed to allocate memory for jv");
      exit(EXIT_FAILURE);
  }
  fread(&value->kind_flags, sizeof(unsigned char), 1, file);
  fread(&value->pad_, sizeof(unsigned char), 1, file);
  fread(&value->offset, sizeof(unsigned short), 1, file);
  fread(&value->size, sizeof(int), 1, file);
  jv_kind kind = jv_get_kind(*value);
  switch (kind) {
    case JV_KIND_NUMBER: {
      int is_lit;
      fread(&is_lit, sizeof(int), 1, file);
      if (!is_lit) {
        fread(&value->u.number, sizeof(double), 1, file);
      } else {
        value->u.ptr = malloc(sizeof(jvp_literal_number));
        if (!value->u.ptr) {
            perror("Failed to allocate memory for jvp_literal_number");
            exit(EXIT_FAILURE);
        }
        jvp_literal_number* lit = (jvp_literal_number*)value->u.ptr;
        fread(&lit->refcnt.count, sizeof(int), 1, file);
        fread(&lit->num_double, sizeof(double), 1, file);

        // Read the flag indicating if literal_data is present
        int has_literal_data;
        fread(&has_literal_data, sizeof(int), 1, file);
        if (has_literal_data) {
            int len;
            fread(&len, sizeof(int), 1, file);
            lit->literal_data = malloc(len + 1);
            if (!lit->literal_data) {
                perror("Failed to allocate memory for literal_data");
                free(value->u.ptr);
                exit(EXIT_FAILURE);
            }
            fread(lit->literal_data, len + 1, 1, file);
        } else {
            lit->literal_data = NULL;
        }
        fread(&lit->num_decimal.digits, sizeof(int32_t), 1, file);
        fread(&lit->num_decimal.exponent, sizeof(int32_t), 1, file);
        fread(&lit->num_decimal.bits, sizeof(uint8_t), 1, file);
        fread(&lit->num_decimal.lsu[0], sizeof(uint16_t), 1, file);
      }
      break;
    }
    case JV_KIND_OBJECT: {
      size_t total_size = sizeof(jvp_object) +
                          sizeof(struct object_slot) * value->size +
                          sizeof(int) * (value->size * 2);
      value->u.ptr = malloc(total_size);
      if (!value->u.ptr) {
          perror("Failed to allocate memory for jvp_object");
          exit(EXIT_FAILURE);
      }
      jvp_object* obj = (jvp_object*)value->u.ptr;
      fread(&obj->refcnt.count, sizeof(int), 1, file);
      deserialize_jv_object(file, obj, value->size);
      fread(&obj->next_free, sizeof(int), 1, file);
      int* hashbuckets = (int*)(&obj->elements[value->size]);
      for (int i=0; i<value->size*2; ++i) {
        fread(&hashbuckets[i], sizeof(int), 1, file);
      }
      value->u.ptr = (struct jv_refcnt*)obj;
      break;
    }
    case JV_KIND_ARRAY: {
      int alloc_len;
      fread(&alloc_len, sizeof(int), 1, file);
      size_t total_size = sizeof(jvp_array) + sizeof(jv) * alloc_len;
      value->u.ptr = malloc(total_size);
      if (!value->u.ptr) {
          perror("malloc");
          fclose(file);
          exit(EXIT_FAILURE);
      }
      jvp_array* arr = (jvp_array*)value->u.ptr;
      fread(&arr->refcnt.count, sizeof(int), 1, file);
      fread(&arr->length, sizeof(int), 1, file);
      fread(&arr->alloc_length, sizeof(int), 1, file);
      deserialize_jv_array(file, arr->length, arr);
      value->u.ptr = (struct jv_refcnt*)arr;
      break;
    }
    case JV_KIND_STRING: {
      int alloc_len;
      fread(&alloc_len, sizeof(int), 1, file);
      size_t total_size = sizeof(jvp_string) + alloc_len + 1;
      value->u.ptr = malloc(total_size);
      if (!value->u.ptr) {
          perror("malloc");
          fclose(file);
          exit(EXIT_FAILURE);
      }
      jvp_string* str = (jvp_string*)value->u.ptr;
      fread(&str->refcnt.count, sizeof(int), 1, file);
      fread(&str->hash, sizeof(uint32_t), 1, file);
      fread(&str->alloc_length, sizeof(uint32_t), 1, file);
      fread(&str->length_hashed, sizeof(uint32_t), 1, file);
      size_t len = str->alloc_length;
      fread(&str->data, len+1, 1, file);
      value->u.ptr = (struct jv_refcnt*)str;
      break;
    }
    case JV_KIND_INVALID: {
      value->u.ptr = malloc(sizeof(jvp_invalid));
      int is_null;
      fread(&is_null, sizeof(int), 1, file);
      if (is_null) {
          value->u.ptr = NULL;
      } else {
          value->u.ptr = malloc(sizeof(jvp_invalid));
          if (!value->u.ptr) {
              perror("Failed to allocate memory for jvp_invalid");
              exit(EXIT_FAILURE);
          }
          jvp_invalid* inv = (jvp_invalid*)value->u.ptr;
          fread(&inv->refcnt.count, sizeof(int), 1, file);
          _deserialize_jv(file, &inv->errmsg);
      }
      break;
    }
    default: {  /* JV_KIND_TRUE, JV_KIND_FALSE, JV_KIND_NULL */
      size_t total_size = sizeof(double);
      if (fread(&value->u, total_size, 1, file) != 1) {
          perror("Failed to read jv value from file");
          fclose(file);
          exit(EXIT_FAILURE);
      }
      break;
    }
  }
}

static struct symbol_table* _deserialize_sym_table(FILE* file) {
  struct symbol_table* table = jv_mem_alloc(sizeof(struct symbol_table));
  fread(&table->ncfunctions, sizeof(int), 1, file);
  _deserialize_jv(file, &table->cfunc_names);
  table->cfunctions = jv_mem_calloc(table->ncfunctions, sizeof(struct cfunction));
  for (int i=0; i<table->ncfunctions; ++i) {
    int len;
    fread(&len, sizeof(int), 1, file);
    table->cfunctions[i].name = malloc(len+1);
    // TODO: Probably don't need a loop for this...
    for (int j=0; j<len+1; ++j) {
      fread((void*)&table->cfunctions[i].name[j], sizeof(char), 1, file);
    }
    fread(&table->cfunctions[i].nargs, sizeof(int), 1, file);
  }
  get_cbindings(table);
  return table;
}

#define MAX_DESERIALIZED_BCS 1000 // Adjust this as needed

static struct bytecode* deserialized_bcs[MAX_DESERIALIZED_BCS];
static int num_deserialized_bcs = 0;

static struct symbol_table* sym = NULL;

static void _set_sym(struct symbol_table* s) {
  sym = s;
}

static struct symbol_table* _get_sym() {
  return sym;
}

static void add_deserialized_bc(struct bytecode* bc) {
    if (num_deserialized_bcs < MAX_DESERIALIZED_BCS) {
        deserialized_bcs[num_deserialized_bcs++] = bc;
    }
}

static struct bytecode* get_deserialized_bc(int idx) {
    if (idx < 0 || idx >= num_deserialized_bcs) {
        return NULL;
    }
    return deserialized_bcs[idx];
}

static struct bytecode* _deserialize_bc(FILE* file);

static struct bytecode* deserialize_bc_parent(FILE* file) {
    uint32_t value;
    fread(&value, sizeof(uint32_t), 1, file);
    if (value == 0xFFFFFFFF) {
        return NULL; // NULL pointer sentinel
    } else if (value == 0xFFFFFFFD) {
        int idx;
        fread(&idx, sizeof(int), 1, file);
        struct bytecode* parent_bc = get_deserialized_bc(idx);
        if (!parent_bc) {
            printf("Error: invalid parent index %d\n", idx);
            exit(EXIT_FAILURE);
        }
        return parent_bc;
    } else {
        fseek(file, -sizeof(uint32_t), SEEK_CUR); // Rewind to read the bytecode struct
        return _deserialize_bc(file);
    }
}


static struct bytecode* _deserialize_bc(FILE* file) {
  /**
   * Precondition: A non-NULL jq->bc struct was serialized
  */

  // Read the codelen to determine if the pointer is null or not
  uint32_t sentinel;
  fread(&sentinel, sizeof(uint32_t), 1, file);
  if (sentinel == 0xFFFFFFFF) {
      return NULL; // NULL pointer sentinel
  } else if (sentinel == 0xFFFFFFFE) {
      // Duplicate pointer detected, do nothing here
      return NULL;
  } else {
      // Not a sentinel, rewind
      fseek(file, -sizeof(uint32_t), SEEK_CUR);
  }
  struct bytecode* bc = (struct bytecode*)malloc(sizeof(struct bytecode));
  if (!bc) {
      perror("Failed to allocate memory for bytecode struct");
      exit(EXIT_FAILURE);
  }
  add_deserialized_bc(bc);
  fread(&bc->codelen, sizeof(int), 1, file); 
  bc->code = (uint16_t*)malloc(sizeof(uint16_t) * bc->codelen);
  if (!bc->code) {
      perror("Failed to allocate memory for bytecode code");
      free(bc); // Free the bytecode struct to avoid a leak
      exit(EXIT_FAILURE);
  }
  fread(bc->code, sizeof(uint16_t), bc->codelen, file);
  fread(&bc->nlocals, sizeof(int), 1, file); 
  fread(&bc->nclosures, sizeof(int), 1, file);
  _deserialize_jv(file, &bc->constants);
  bc->parent = deserialize_bc_parent(file);
  if (!bc->parent) {
    bc->globals = _deserialize_sym_table(file);
    _set_sym(bc->globals);
  }
  else {
    bc->globals = _get_sym();
  }

  // Deserialize subfunctions recursively
  fread(&bc->nsubfunctions, sizeof(int), 1, file);
  if (bc->nsubfunctions > 0) {
    bc->subfunctions = malloc(bc->nsubfunctions * sizeof(struct bytecode*));
    if (!bc->subfunctions) {
        perror("Failed to allocate memory for subfunctions");
        free(bc->code); // Free allocated memory for code
        free(bc); // Free the bytecode struct to avoid a leak
        exit(EXIT_FAILURE);
    }
    if (!bc->subfunctions) {
        printf("Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
  }
  else
    bc->subfunctions = NULL;
  for (int i = 0; i<bc->nsubfunctions; ++i) {
      bc->subfunctions[i] = _deserialize_bc(file);
  }
  _deserialize_jv(file, &bc->debuginfo);
  return bc;
}

static struct bytecode* deserialize_bc(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        perror("Failed to open file for reading");
        return NULL;
    }
    struct bytecode* bc = _deserialize_bc(file);
    fclose(file);
    return bc;
}


static void debug_cb(void *data, jv input) {
  int dumpopts = *(int *)data;
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
int cjq_bootstrap(int argc, char* argv[], cjq_state *cjq) {
  #endif
  jq_state* jq = NULL;
  jq_util_input_state* cjq_input_state = NULL;
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

  cjq_input_state = jq_util_input_init(NULL, NULL); // XXX add err_cb

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
        jq_util_input_add_input(cjq_input_state, argv[i]);
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
  }
  
  struct bytecode* bc = deserialize_bc("serialize.bin");    // TODO: Not a long-term solution
  jv_nomem_handler(jq->nomem_handler, jq->nomem_handler_data);
  _jq_reset(jq);
  jq->bc = bc;
  if (options & DUMP_DISASM) {
    jq_dump_disassembly(jq, 0);
    printf("\n");  
  }

  if ((options & SEQ))
    parser_flags |= JV_PARSE_SEQ;

  if ((options & RAW_INPUT)) {
    jq_util_input_set_parser(cjq_input_state, NULL, (options & SLURP) ? 1 : 0);
  }
  else {
    jq_util_input_set_parser(cjq_input_state, jv_parser_new(parser_flags), (options & SLURP) ? 1 : 0);
  }

  // Let jq program read from inputs
  jq_set_input_cb(jq, jq_util_input_next_input_cb, cjq_input_state);

  // Let jq program call `debug` builtin and have that go somewhere
  jq_set_debug_cb(jq, debug_cb, &dumpopts);

  // Let jq program call `stderr` builtin and have that go somewhere
  jq_set_stderr_cb(jq, stderr_cb, &dumpopts);

  if (nfiles == 0) {
    jq_util_input_add_input(cjq_input_state, "-");
  }

 
out:
  badwrite = ferror(stdout);
// JOHN: Commented below code out because it was preventing further printing
//   if (fclose(stdout)!=0 || badwrite) {   // JOHN: Specifically, fclose(stdout)
//     fprintf(stderr,"jq: error: writing output failed: %s\n", strerror(errno));
//     ret = JQ_ERROR_SYSTEM;
//   }

  if (options & PROVIDE_NULL)
    cjq_init(cjq, ret, jq_flags, options, dumpopts, last_result, NULL, jq);
  else
    cjq_init(cjq, ret, jq_flags, options, dumpopts, last_result, cjq_input_state, jq);

  jv_free(ARGS);
  jv_free(program_arguments);

  return 0;
}
