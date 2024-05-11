#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../clib/lib.h"

#include "../jq/src/compile.h"
#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/util.h"
#include "../jq/src/version.h"
#include "../frontend/cjq_frontend.h"

#define jq_exit(r)              exit( r > 0 ? r : 0 )

extern void jq_program();

void clean_up(compiled_jq_state* cjq_state) {
    jq_util_input_free(&(cjq_state->input_state));
    jq_teardown(&(cjq_state->jq));
    cjq_free(cjq_state);
}

int main(int argc, char *argv[]) {
  compiled_jq_state *cjq_state = malloc(sizeof(compiled_jq_state));
  // TODO: Refactor such that we don't compile a second time (just need data such as bc->constants)
  int parse_error = cjq_parse(argc, argv, cjq_state);
  jq_program((void*)cjq_state);
  clean_up(cjq_state);
  return 0;
}