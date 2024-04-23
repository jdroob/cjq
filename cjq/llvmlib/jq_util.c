#include "../frontend/src/cjq_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PY_SSIZE_T_CLEAN
#include <../../usr/include/python3.12/Python.h>      // TODO: yuck
#include <../../usr/include/python3.12/pyconfig.h>      // TODO: yuck

void _cjq_execute() {
    // Call cjq_execute
    cjq_execute(cjq_state.jq, cjq_state.input_state, cjq_state.jq_flags,
                 cjq_state.dumpopts, cjq_state.options, cjq_state.ret,
                 cjq_state.last_result, cjq_state.opcode_list, 
                 cjq_state.opcode_list_len, 0);
}

uint16_t _get_opcode(void* cjq_state_ptr) {
    // NOTE: bytecode instructions have variable length
    //       - will need to account for this
    printf("Made it to _get_opcode()\n");
    printf("Pointer passed to _get_opcode(): %p\n", (compiled_jq_state*)cjq_state_ptr);
    printf("current pc: %i\n", *((compiled_jq_state*)cjq_state_ptr)->pc);
    return *((compiled_jq_state*)cjq_state_ptr)->pc;
    // uint16_t opcode = *((compiled_jq_state*)cjq_state_ptr)->pc;
    // cjq_state.pc++;
    // return opcode;
}

// Step 1: Run Python script & write to ir.ll
// Step 2: Compile main.c with ir.ll
// Step 3: Run the executable
