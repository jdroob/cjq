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
#include "cjq_bootstrap.h"

extern void jq_program();

static void clean_up(cjq_state* cjq) {
    jq_util_input_free(&(cjq->input_state));
    free_cfunction_names(cjq->jq->bc);
    jq_teardown(&(cjq->jq));
    cjq_free(cjq);
}

int main(int argc, char *argv[]) {
  cjq_state* cjq = cjq_mem_alloc();

  // set/get information req'd for execution
  int bootstrap_error = cjq_bootstrap(argc, argv, cjq);
  
  // run the jq program
  jq_program((void*)cjq);

  // store exit data prior to freeing cjq
  int options = *cjq->options;
  int ret = *cjq->ret;
  if (cjq->input_state != NULL && jq_util_input_errors(cjq->input_state) != 0)
    ret = JQ_ERROR_SYSTEM;
  int last_result = *cjq->last_result;
  clean_up(cjq);

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