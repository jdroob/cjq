#include <stdlib.h>

#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#ifndef CJQ_TRACE_H
#define CJQ_TRACE_H

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
    (type*)reallocate(pointer, sizeof(type) * (old_count), \
        sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) \
    reallocate(pointer, sizeof(type) * (old_count), 0)


typedef struct {
    int count;
    int capacity;
    uint8_t* ops;
} opcode_list;

typedef struct {
    int count;
    int capacity;
    uint16_t* entry_locs;
} jq_next_entry_list;

typedef struct {
    int count;
    int capacity;
    uint16_t* input_locs;
} jq_next_input_list;

typedef struct {
    opcode_list* opcodes;
    jq_next_entry_list* entries;
    jq_next_input_list* inputs;
    int* jq_halt_loc;
} trace;

trace* init_trace();
void update_opcode_list(trace* opcode_trace, uint8_t opcode);
void update_entry_list(trace* opcode_trace);
void update_input_list(trace* opcode_trace);
void update_halt_loc(trace* opcode_trace);
void free_trace(trace* opcode_trace);
int cjq_trace(int argc, char* argv[], trace* opcode_trace);

#endif  /* CJQ_TRACE_H */