#include "../frontend/src/cjq_frontend.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
