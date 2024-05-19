#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../frontend/cjq_frontend.h"
#include "../trace/cjq_trace.h"
#include "../jq/src/bytecode.h"


uint16_t _get_opcode(void* cjq_state_ptr) {
    // Deprecated?
    return *((compiled_jq_state*)cjq_state_ptr)->pc;
}

int _get_opcode_list_len(void* opcodes_ptr) {
    // printf("Made it to _get_opcode_list_len\n");
    return *((trace*)opcodes_ptr)->opcode_list_len;
}

uint8_t _opcode_list_at(void* opcodes_ptr, int index) {
    // printf("Made it to _opcode_list_at\n");
    trace* popcodes_ptr = (trace*)opcodes_ptr;
    return ((trace*)opcodes_ptr)->opcode_list[index];
}

int _get_jq_next_entry_list_len(void* opcodes_ptr) {
    return *((trace*)opcodes_ptr)->jq_next_entry_list_len;
}

uint16_t _jq_next_entry_list_at(void* opcodes_ptr, int index) {
    // printf("Made it to _jq_next_entry_list_at\n");
    return ((trace*)opcodes_ptr)->jq_next_entry_list[index];
}

int _get_num_opcodes() {
    // printf("In _get_num_opcodes\n");
    return NUM_OPCODES;
}

int _get_jq_halt_loc(void* opcodes_ptr) {
    return *((trace*)opcodes_ptr)->jq_halt_loc;
}
