/* A collection of C functions that perform the same actions as each
   opcode case found in execute.c > jq_next.
   
   All side-effects to cjq_state attributes are exactly the same.
*/
#include "../frontend/src/cjq_frontend.h"


void _cjq_execute() {
    // Call cjq_execute
    cjq_execute(cjq_state.jq, cjq_state.input_state, cjq_state.jq_flags,
                 cjq_state.dumpopts, cjq_state.options, cjq_state.ret,
                 cjq_state.last_result, cjq_state.opcode_list, 
                 cjq_state.opcode_list_len, 0);
}