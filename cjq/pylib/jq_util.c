#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../bootstrap/cjq_bootstrap.h"
#include "../trace/cjq_trace.h"
#include "../jq/src/bytecode.h"


uint16_t _get_opcode(void* cjq) {
    // Deprecated?
    return *((cjq_state*)cjq)->pc;
}

uint64_t _get_opcode_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->opcodes->count;
}

uint8_t _opcode_list_at(void* opcode_trace_ptr, uint64_t index) {
    trace* popcode_trace_ptr = (trace*)opcode_trace_ptr;
    printf("in get opcode: %hhu\n", popcode_trace_ptr->opcodes->ops[index]);
    return popcode_trace_ptr->opcodes->ops[index];
}

uint64_t _get_jq_next_entry_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->entries->count;
}

uint64_t _jq_next_entry_list_at(void* opcode_trace_ptr, uint64_t index) {
    return ((trace*)opcode_trace_ptr)->entries->entry_locs[index];
}

uint64_t _get_next_input_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->inputs->count;
}

uint64_t _next_input_list_at(void* opcode_trace_ptr, uint64_t index) {
    return ((trace*)opcode_trace_ptr)->inputs->input_locs[index];
}

int _get_num_opcodes() {
    return NUM_OPCODES;
}

uint64_t _get_jq_halt_loc(void* opcode_trace_ptr) {
    // printf("In jq_util, jq_halt_loc: %lu", ((trace*)opcode_trace_ptr)->jq_halt_loc);
    return ((trace*)opcode_trace_ptr)->jq_halt_loc;
}
