#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../jq/src/compile.h"
#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../jq/src/jv_alloc.h"
#include "../jq/src/util.h"
#include "../jq/src/version.h"
#include "../jq/src/common.h"
#include "cjq_runtime.h"

extern void jq_program();

void clean_up(compiled_jq_state* cjq_state) {
    jq_util_input_free(&(cjq_state->input_state));
    free_cfunction_names(cjq_state->jq->bc);
    jq_teardown(&(cjq_state->jq));
    cjq_free(cjq_state);
}

int main(int argc, char *argv[]) {
  compiled_jq_state* cjq_state = malloc(sizeof(compiled_jq_state));
  int parse_error = cjq_run(argc, argv, cjq_state);
  
  jq_program((void*)cjq_state);

  // Store exit data prior to freeing cjq_state
  int options = *cjq_state->options;
  int ret = *cjq_state->ret;
  if (cjq_state->input_state != NULL && jq_util_input_errors(cjq_state->input_state) != 0)
    ret = JQ_ERROR_SYSTEM;
  int last_result = *cjq_state->last_result;
  clean_up(cjq_state);

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