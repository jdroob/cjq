#ifndef FRONTEND_MAIN_H
#define FRONTEND_MAIN_H

typedef struct {
    jv ARGS;
    jv program_arguments;
    jq_util_input_state *input_state;
    jq_state* jq;
} compiled_jq_state;

compiled_jq_state frontend_main(int argc, char *argv[]);

#endif  /* FRONTEND_MAIN_H */
