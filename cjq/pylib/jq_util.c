#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../runtime/cjq_runtime.h"
#include "../trace/cjq_trace.h"
#include "../jq/src/bytecode.h"


uint16_t _get_opcode(void* cjq_state_ptr) {
    // Deprecated?
    return *((compiled_jq_state*)cjq_state_ptr)->pc;
}

int _get_opcode_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->opcodes->count;
}

uint8_t _opcode_list_at(void* opcode_trace_ptr, int index) {
    trace* popcode_trace_ptr = (trace*)opcode_trace_ptr;
    return popcode_trace_ptr->opcodes->ops[index];
}

int _get_jq_next_entry_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->entries->count;
}

uint16_t _jq_next_entry_list_at(void* opcode_trace_ptr, int index) {
    return ((trace*)opcode_trace_ptr)->entries->entry_locs[index];
}

int _get_next_input_list_len(void* opcode_trace_ptr) {
    return ((trace*)opcode_trace_ptr)->inputs->count;
}

uint16_t _next_input_list_at(void* opcode_trace_ptr, int index) {
    return ((trace*)opcode_trace_ptr)->inputs->input_locs[index];
}

int _get_num_opcodes() {
    return NUM_OPCODES;
}

int _get_jq_halt_loc(void* opcode_trace_ptr) {
    return *((trace*)opcode_trace_ptr)->jq_halt_loc;
}
