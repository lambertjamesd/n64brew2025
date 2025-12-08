#include "expression.h"

#include <libdragon.h>
#include <assert.h>
#include <malloc.h>

void expression_load(struct expression* expression, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_EXPR_HEADER);
    uint16_t byte_size;
    fread(&byte_size, 1, 2, file);
    expression->expression_program = malloc(byte_size);
    fread(expression->expression_program, 1, byte_size, file);
}

void expression_destroy(struct expression* expression) {
    free(expression->expression_program);
    expression->expression_program = NULL;
}

void expression_load_literal(struct expression* expression, int literal) {
    expression->expression_program = malloc(6);
    char* program = expression->expression_program;
    ((char*)program)[0] = EXPRESSION_TYPE_LOAD_LITERAL;
    memcpy(&program[1], &literal, 4);
    program[5] = EXPRESSION_TYPE_END;
}

void expression_builder_add(expression_builder_t* builder, enum expression_type type, union expression_data* data) {
    assert(builder->curr < &builder->expression[MAX_EXPRESSION_SIZE]);

    *builder->curr = type;
    ++builder->curr;

    if (data) {
        assert(builder->curr + sizeof(union expression_data) <= &builder->expression[MAX_EXPRESSION_SIZE]);
        memcpy(builder->curr, data, sizeof(union expression_data));
        builder->curr += sizeof(union expression_data);
    }
}

void expression_builder_finish(expression_builder_t* builder, expression_t* expression) {
    int len = builder->curr - builder->expression;
    char* program = malloc(len + 1);
    memcpy(program, builder->expression, len);
    program[len] = EXPRESSION_TYPE_END;
    expression->expression_program = program;
}