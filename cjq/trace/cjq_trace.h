#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#ifndef CJQ_TRACE_H
#define CJQ_TRACE_H

typedef struct {
    int* ret;
    int* jq_flags;
    int* dumpopts;
    int* options;
    int* last_result;
    int* raising;
    jv* value;
    jv* result;
    jv* cfunc_input;
    jq_state* jq;
    uint16_t* pc;
    uint16_t* opcode;
    int* backtracking;
    uint8_t* fallthrough;
} compiled_jq_state;

typedef struct {
    uint8_t* opcode_list;
    int* opcode_list_len;
    uint16_t* jq_next_entry_list;
    int* jq_next_entry_list_len;
    int* jq_halt_loc;
    uint16_t* jq_next_input_list;
    int* jq_next_input_list_len;
} trace;

void trace_init(trace* opcodes, uint8_t* opcode_list, int opcode_list_len, 
                uint16_t* jq_next_entry_list, int jq_next_entry_list_len,
                int jq_halt_loc, uint16_t* jq_next_input_list, int jq_next_input_list_len);

int cjq_trace(int argc, char* argv[], trace* opcodes, compiled_jq_state* cjq_state);

void cjq_init(compiled_jq_state* cjq_state, int ret, int jq_flags, int options,
             int dumpopts, int last_result, jv* value, jq_state* jq);

void cjq_free(compiled_jq_state* cjq_state);

#endif  /* CJQ_TRACE_H */