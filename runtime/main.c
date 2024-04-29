#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../cjq/clib/lib.h"

#include "../cjq/frontend/src/compile.h"
#include "../cjq/frontend/src/jv.h"
#include "../cjq/frontend/src/jq.h"
#include "../cjq/frontend/src/jv_alloc.h"
#include "../cjq/frontend/src/util.h"
#include "../cjq/frontend/src/version.h"
#include "../cjq/frontend/src/cjq_frontend.h"

#define jq_exit(r)              exit( r > 0 ? r : 0 )

// Globals
compiled_jq_state cjq_state;
extern void jq_program();

void clean_up(compiled_jq_state* cjq_state) {
    // Free memory allocated by cjq_frontend
    jq_util_input_free(&(cjq_state->input_state));
    jq_teardown(&(cjq_state->jq));
    cjq_free();
}

int main(int argc, char *argv[]) {
  // Parse source code & prepare setup for lowering
  // TODO: Eventually, we want this to only gather necessary information on JSON data
  //       and to NOT re-compile to bytecode since by this point we'll already
  //       have the LLVM that contains a sequence of opcode-function calls
  int parse_error = cjq_parse(argc, argv);    // This call really just needs to populate cjq_state (i.e. get bc->constants, etc. and get data pushed to stack)
  // TODO: This program should take in path(s) to JSON file(s)
  // TODO: These JSON file(s) should be passed to jqlib function that
  //       sets up requisite data structures and converts JSON data
  //       to proper type (some jv value or it's pushed onto the stack or something like that)
  // TODO: Converted JSON data should be passed to jq_program function
  jq_program();

  // Free up resources
  clean_up(&cjq_state);     // TODO: Fixme
  return 0;
}