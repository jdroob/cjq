#include "jv.h"
#include <jq.h>
#ifndef CJQ_FRONTEND_H
#define CJQ_FRONTEND_H

typedef struct {
    int* ret;
    int* jq_flags;
    int* dumpopts;
    int* options;
    int* last_result;
    uint8_t* opcode_list;
    int* opcode_list_len;
    jq_util_input_state *input_state;
    jq_state* jq;
    uint16_t* pc;
} compiled_jq_state;

extern compiled_jq_state cjq_state;

void cjq_init(int ret, int jq_flags, int options, int dumpopts, int last_result, int opcode_list_len,
              uint8_t* opcode_list, jq_util_input_state* input_state, jq_state* jq,
              uint16_t* pc);

void cjq_execute(jq_state* jq, jq_util_input_state* input_state, 
                  int* jq_flags, int* dumpopts, int* options, int* ret, int* last_result,
                  uint8_t* opcode_list, int* opcode_list_len, int tracing);


int cjq_parse(int argc, char *argv[]);

#endif  /* CJQ_FRONTEND_H */
