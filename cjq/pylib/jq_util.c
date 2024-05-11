#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../frontend/cjq_frontend.h"
#include "../trace/cjq_trace.h"
#include "../jq/src/bytecode.h"


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

int _get_opcode_list_len(void* ocodes_ptr) {
    // printf("Made it to _get_opcode_list_len\n");
    return *((trace*)ocodes_ptr)->opcode_list_len;
}

uint8_t _opcode_list_at(void* ocodes_ptr, int index) {
    // printf("Made it to _opcode_list_at\n");
    return ((trace*)ocodes_ptr)->opcode_list[index];
}

int _get_num_opcodes() {
    // printf("In _get_num_opcodes\n");
    return NUM_OPCODES;
}
