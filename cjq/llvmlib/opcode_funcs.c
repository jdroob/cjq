/* A collection of C functions that perform the same actions as each
   opcode case found in execute.c > jq_next.
   
   All side-effects to cjq_state attributes are exactly the same.
*/
#include "../frontend/cjq_frontend.h"


void _cjq_execute() {
   // DEPRECATED
   
    // Call cjq_execute
   //  cjq_execute(cjq_state.jq, cjq_state.input_state, cjq_state.jq_flags,
   //               cjq_state.dumpopts, cjq_state.options, cjq_state.ret,
   //               cjq_state.last_result, cjq_state.opcode_list, 
   //               cjq_state.opcode_list_len, 0);
}

void _opcode_TOP() { /* This opcode does nothing */ }

void _opcode_BACKTRACK_TOP() {}

void _opcode_SUBEXP_BEGIN() {}

void _opcode_BACKTRACK_SUBEXP_BEGIN() {}

void _opcode_PUSHK_UNDER() {}

void _opcode_BACKTRACK_PUSHK_UNDER() {}

void _opcode_INDEX() {}

void _opcode_BACKTRACK_INDEX() {}

void _opcode_SUBEXP_END() {}

void _opcode_BACKTRACK_SUBEXP_END() {}

void _opcode_CALL_BUILTIN_plus() {}

void _opcode_BACKTRACK_CALL_BUILTIN_plus() {}

void _opcode_RET() {}

void _opcode_BACKTRACK_RET() {}