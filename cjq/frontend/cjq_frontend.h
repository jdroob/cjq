#include "../jq/src/jv.h"
#include "../jq/src/jq.h"
#include "../trace/cjq_trace.h"
#ifndef CJQ_FRONTEND_H
#define CJQ_FRONTEND_H

// typedef struct {
//     int* ret;
//     int* jq_flags;
//     int* dumpopts;
//     int* options;
//     int* last_result;
//     int* raising;
//     jq_util_input_state* input_state;
//     jv* value;
//     jv* result;
//     jv* cfunc_input;
//     jq_state* jq;
//     uint16_t* pc;
//     uint16_t* opcode;
//     int* backtracking;
//     uint8_t* fallthrough;
// } compiled_jq_state;

// void cjq_init(compiled_jq_state* cjq_state, int ret, int jq_flags, int options,
//              int dumpopts, int last_result, jq_util_input_state* input_state, 
//              jq_state* jq);

// void cjq_free(compiled_jq_state* cjq_state);

// int cjq_parse(int argc, char* argv[], compiled_jq_state* cjq_state);

void _opcode_LOADK(void* cjq_state);
void _opcode_DUP(void* cjq_state);
void _opcode_DUPN(void* cjq_state);
void _opcode_DUP2(void* cjq_state);
void _opcode_PUSHK_UNDER(void* cjq_state);
void _opcode_POP(void* cjq_state);
void _opcode_LOADV(void* cjq_state);
void _opcode_LOADVN(void* cjq_state);
void _opcode_STOREV(void* cjq_state);
void _opcode_STORE_GLOBAL(void* cjq_state);
void _opcode_INDEX(void* cjq_state);
void _opcode_INDEX_OPT(void* cjq_state);
void _opcode_EACH(void* cjq_state);
void _opcode_BACKTRACK_EACH(void* cjq_state);
void _opcode_EACH_OPT(void* cjq_state);
void _opcode_BACKTRACK_EACH_OPT(void* cjq_state);
void _opcode_FORK(void* cjq_state);
void _opcode_BACKTRACK_FORK(void* cjq_state);
void _opcode_TRY_BEGIN(void* cjq_state);
void _opcode_BACKTRACK_TRY_BEGIN(void* cjq_state);
void _opcode_TRY_END(void* cjq_state);
void _opcode_BACKTRACK_TRY_END(void* cjq_state);
void _opcode_JUMP(void* cjq_state);
void _opcode_JUMP_F(void* cjq_state);
void _opcode_BACKTRACK(void* cjq_state);
void _opcode_APPEND(void* cjq_state);
void _opcode_INSERT(void* cjq_state);
void _opcode_RANGE(void* cjq_state);
void _opcode_BACKTRACK_RANGE(void* cjq_state);
void _opcode_SUBEXP_BEGIN(void* cjq_state);
void _opcode_SUBEXP_END(void* cjq_state);
void _opcode_PATH_BEGIN(void* cjq_state);
void _opcode_BACKTRACK_PATH_BEGIN(void* cjq_state);
void _opcode_PATH_END(void* cjq_state);
void _opcode_BACKTRACK_PATH_END(void* cjq_state);
void _opcode_CALL_BUILTIN(void* cjq_state);
void _opcode_CALL_JQ(void* cjq_state);
void _opcode_RET(void* cjq_state);
void _opcode_BACKTRACK_RET(void* cjq_state);
void _opcode_TAIL_CALL_JQ(void* cjq_state);
void __attribute__((always_inline)) inline _opcode_TOP(void* cjq_state);
void _opcode_GENLABEL(void* cjq_state);
void _opcode_DESTRUCTURE_ALT(void* cjq_state);
void _opcode_BACKTRACK_DESTRUCTURE_ALT(void* cjq_state);
void _opcode_STOREVN(void* cjq_state);
void _opcode_BACKTRACK_STOREVN(void* cjq_state);
void _opcode_ERRORK(void* cjq_state);

#endif  /* CJQ_FRONTEND_H */
