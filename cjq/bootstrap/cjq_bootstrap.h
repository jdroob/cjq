#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../trace/cjq_trace.h"
#ifndef CJQ_BOOTSTRAP_H
#define CJQ_BOOTSTRAP_H

typedef struct {
    int* ret;
    int* jq_flags;
    int* dumpopts;
    int* options;
    int* last_result;
    int* raising;
    jq_util_input_state* input_state;
    jv* value;
    jv* result;
    jq_state* jq;
    uint16_t* pc;
    uint16_t* opcode;
    int* backtracking;
    uint8_t* fallthrough;
} cjq_state;

void cjq_init(cjq_state* cjq, int ret, int jq_flags, int options,
             int dumpopts, int last_result, jq_util_input_state* input_state, 
             jq_state* jq);

void free_cfunction_names(void* bc);
void cjq_free(cjq_state* cjq);
int cjq_bootstrap(int argc, char* argv[], cjq_state* cjq);
cjq_state* cjq_mem_alloc();
void __attribute__((always_inline)) inline lg_init(cjq_state* cjq);
void __attribute__((always_inline)) inline _init_jq_next(void* cjq);
void __attribute__((always_inline)) inline _do_backtrack(cjq_state *cjq);
void __attribute__((always_inline)) inline _opcode_LOADK(void* cjq);
void __attribute__((always_inline)) inline _opcode_DUP(void* cjq);
void __attribute__((always_inline)) inline _opcode_DUPN(void* cjq);
void __attribute__((always_inline)) inline _opcode_DUP2(void* cjq);
void __attribute__((always_inline)) inline _opcode_PUSHK_UNDER(void* cjq);
void __attribute__((always_inline)) inline _opcode_POP(void* cjq);
void __attribute__((always_inline)) inline _opcode_LOADV(void* cjq);
void __attribute__((always_inline)) inline _opcode_LOADVN(void* cjq);
void __attribute__((always_inline)) inline _opcode_STOREV(void* cjq);
void __attribute__((always_inline)) inline _opcode_STORE_GLOBAL(void* cjq);
void __attribute__((always_inline)) inline _opcode_INDEX(void* cjq);
void __attribute__((always_inline)) inline _opcode_INDEX_OPT(void* cjq);
void __attribute__((always_inline)) inline _opcode_EACH(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_EACH(void* cjq);
void __attribute__((always_inline)) inline _opcode_EACH_OPT(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_EACH_OPT(void* cjq);
void __attribute__((always_inline)) inline _opcode_FORK(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_FORK(void* cjq);
void __attribute__((always_inline)) inline _opcode_TRY_BEGIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_TRY_BEGIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_TRY_END(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_TRY_END(void* cjq);
void __attribute__((always_inline)) inline _opcode_JUMP(void* cjq);
void __attribute__((always_inline)) inline _opcode_JUMP_F(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK(void* cjq);
void __attribute__((always_inline)) inline _opcode_APPEND(void* cjq);
void __attribute__((always_inline)) inline _opcode_INSERT(void* cjq);
void __attribute__((always_inline)) inline _opcode_RANGE(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_RANGE(void* cjq);
void __attribute__((always_inline)) inline _opcode_SUBEXP_BEGIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_SUBEXP_END(void* cjq);
void __attribute__((always_inline)) inline _opcode_PATH_BEGIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_PATH_BEGIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_PATH_END(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_PATH_END(void* cjq);
void __attribute__((always_inline)) inline _opcode_CALL_BUILTIN(void* cjq);
void __attribute__((always_inline)) inline _opcode_CALL_JQ(void* cjq);
void __attribute__((always_inline)) inline _opcode_RET(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_RET(void* cjq);
void __attribute__((always_inline)) inline _opcode_TAIL_CALL_JQ(void* cjq);
void __attribute__((always_inline)) inline _opcode_TOP(void* cjq);
void __attribute__((always_inline)) inline _opcode_GENLABEL(void* cjq);
void __attribute__((always_inline)) inline _opcode_DESTRUCTURE_ALT(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_DESTRUCTURE_ALT(void* cjq);
void __attribute__((always_inline)) inline _opcode_STOREVN(void* cjq);
void __attribute__((always_inline)) inline _opcode_BACKTRACK_STOREVN(void* cjq);
void __attribute__((always_inline)) inline _opcode_ERRORK(void* cjq);

#endif  /* CJQ_BOOTSTRAP_H */
