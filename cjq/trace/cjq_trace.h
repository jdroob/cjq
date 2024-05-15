#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#ifndef CJQ_TRACE_H
#define CJQ_TRACE_H

typedef struct {
    uint8_t* opcode_list;
    int* opcode_list_len;
    uint16_t* nprint_loops;
} trace;

void trace_init(trace *opcodes, uint8_t *opcode_list, int opcode_list_len, uint16_t nprint_loops);
int cjq_trace(int argc, char *argv[], trace *opcodes);

#endif  /* CJQ_TRACE_H */
