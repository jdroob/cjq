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
// inline void lg_init(cjq_state* cjq);
inline void _init_jq_next(void* cjq);
// inline void _do_backtrack(cjq_state *cjq);
inline void _opcode_LOADK(void* cjq);
inline void _opcode_DUP(void* cjq);
inline void _opcode_DUPN(void* cjq);
inline void _opcode_DUP2(void* cjq);
inline void _opcode_PUSHK_UNDER(void* cjq);
inline void _opcode_POP(void* cjq);
inline void _opcode_LOADV(void* cjq);
inline void _opcode_LOADVN(void* cjq);
inline void _opcode_STOREV(void* cjq);
inline void _opcode_STORE_GLOBAL(void* cjq);
inline void _opcode_INDEX(void* cjq);
inline void _opcode_INDEX_OPT(void* cjq);
inline void _opcode_EACH(void* cjq);
inline void _opcode_BACKTRACK_EACH(void* cjq);
inline void _opcode_EACH_OPT(void* cjq);
inline void _opcode_BACKTRACK_EACH_OPT(void* cjq);
inline void _opcode_FORK(void* cjq);
inline void _opcode_BACKTRACK_FORK(void* cjq);
inline void _opcode_TRY_BEGIN(void* cjq);
inline void _opcode_BACKTRACK_TRY_BEGIN(void* cjq);
inline void _opcode_TRY_END(void* cjq);
inline void _opcode_BACKTRACK_TRY_END(void* cjq);
inline void _opcode_JUMP(void* cjq);
inline void _opcode_JUMP_F(void* cjq);
inline void _opcode_BACKTRACK(void* cjq);
inline void _opcode_APPEND(void* cjq);
inline void _opcode_INSERT(void* cjq);
inline void _opcode_RANGE(void* cjq);
inline void _opcode_BACKTRACK_RANGE(void* cjq);
inline void _opcode_SUBEXP_BEGIN(void* cjq);
inline void _opcode_SUBEXP_END(void* cjq);
inline void _opcode_PATH_BEGIN(void* cjq);
inline void _opcode_BACKTRACK_PATH_BEGIN(void* cjq);
inline void _opcode_PATH_END(void* cjq);
inline void _opcode_BACKTRACK_PATH_END(void* cjq);
inline void _opcode_CALL_BUILTIN(void* cjq);
inline void _opcode_CALL_JQ(void* cjq);
inline void _opcode_RET(void* cjq);
inline void _opcode_BACKTRACK_RET(void* cjq);
inline void _opcode_TAIL_CALL_JQ(void* cjq);
inline void _opcode_TOP(void* cjq);
inline void _opcode_GENLABEL(void* cjq);
inline void _opcode_DESTRUCTURE_ALT(void* cjq);
inline void _opcode_BACKTRACK_DESTRUCTURE_ALT(void* cjq);
inline void _opcode_STOREVN(void* cjq);
inline void _opcode_BACKTRACK_STOREVN(void* cjq);
inline void _opcode_ERRORK(void* cjq);

#endif  /* CJQ_BOOTSTRAP_H */
