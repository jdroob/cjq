#include "../frontend/src/cjq_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void _cjq_execute() {
    // Call cjq_execute
    cjq_execute(cjq_state.jq, cjq_state.input_state, cjq_state.jq_flags,
                 cjq_state.dumpopts, cjq_state.options, cjq_state.ret,
                 cjq_state.last_result, cjq_state.opcode_list, 
                 cjq_state.opcode_list_len, 0);
}

// Step 1: Run Python script & write to ir.ll
// Step 2: Compile main.c with ir.ll
// Step 3: Run the executable
