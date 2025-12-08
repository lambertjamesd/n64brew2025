#ifndef __CUTSCENE_EXPRESSION_H__
#define __CUTSCENE_EXPRESSION_H__

#include <stdint.h>
#include <libdragon.h>

enum expression_type {
    EXPRESSION_TYPE_END,
    EXPRESSION_TYPE_LOAD_LOCAL,
    EXPRESSION_TYPE_LOAD_SCENE_VAR,
    EXPRESSION_TYPE_LOAD_GLOBAL,
    EXPRESSION_TYPE_LOAD_LITERAL,

    EXPRESSION_TYPE_AND,
    EXPRESSION_TYPE_OR,
    EXPRESSION_TYPE_NOT,

    EXPRESSION_TYPE_EQ,
    EXPRESSION_TYPE_NEQ,
    EXPRESSION_TYPE_GT,
    EXPRESSION_TYPE_GTE,

    EXPRESSION_TYPE_ADD,
    EXPRESSION_TYPE_SUB,
    EXPRESSION_TYPE_MUL,
    EXPRESSION_TYPE_DIV,
    EXPRESSION_TYPE_NEGATE,

    EXPRESSION_TYPE_GTF,
    EXPRESSION_TYPE_GTEF,

    EXPRESSION_TYPE_ADDF,
    EXPRESSION_TYPE_SUBF,
    EXPRESSION_TYPE_MULF,
    EXPRESSION_TYPE_DIVF,
    EXPRESSION_TYPE_NEGATEF,

    EXPRESSION_TYPE_ITOF,
    EXPRESSION_TYPE_FTOI,

    EXPRESSION_TYPE_BUILT_IN_FN,
};

union expression_data {
    struct {
        uint16_t data_type;
        uint16_t word_offset;
    } load_variable;
    struct {
        uint16_t fn;
        uint8_t arg_count;
        uint8_t result_count;
    } fn_call;
    int literal;
};

typedef union expression_data expression_data_t;

struct expression {
    void* expression_program;
};

typedef struct expression expression_t;

// EXPR
#define EXPECTED_EXPR_HEADER 0x45585052

struct __attribute__((packed)) expression_header {
    uint32_t header;
    uint16_t len;
};

void expression_load(struct expression* expression, FILE* file);
void expression_destroy(struct expression* expression);

void expression_load_literal(struct expression* expression, int literal);

#define MAX_EXPRESSION_SIZE 64

struct expression_builder {
    char* curr;
    char expression[MAX_EXPRESSION_SIZE];
};

typedef struct expression_builder expression_builder_t;

static inline void expression_builder_init(expression_builder_t* builder) {
    builder->curr = builder->expression;
}

void expression_builder_add(expression_builder_t* builder, enum expression_type type, union expression_data* data);

static inline void expression_builder_load_literal(expression_builder_t* builder, int value) {
    expression_builder_add(builder, EXPRESSION_TYPE_LOAD_LITERAL, &(expression_data_t) {
        .literal = value,
    });
}

void expression_builder_finish(expression_builder_t* builder, expression_t* expression);

#endif