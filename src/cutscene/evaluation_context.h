#ifndef __CUTSCENE_EVALUATION_CONTEXT_H__
#define __CUTSCENE_EVALUATION_CONTEXT_H__

#define MAX_STACK_SIZE  128

#include <stdint.h>

enum data_type {
    DATA_TYPE_NULL,
    DATA_TYPE_S8,
    DATA_TYPE_S16,
    DATA_TYPE_S32,

    DATA_TYPE_BOOL,

    DATA_TYPE_F32,

    DATA_TYPE_ADDRESS,
};

typedef enum data_type data_type_t;

struct evaluation_context {
    int stack[MAX_STACK_SIZE];
    uint16_t current_stack;
    void* local_varaibles;
};

typedef struct evaluation_context evaluation_context_t;

void evaluation_context_init(struct evaluation_context* context, int locals_size);
void evaluation_context_destroy(struct evaluation_context* context);

void evaluation_context_push(struct evaluation_context* context, int value);
int evaluation_context_pop(struct evaluation_context* context);
void evaluation_context_popn(struct evaluation_context* context, int* into, int count);
int evaluation_context_peek(struct evaluation_context* context);

int evaluation_context_load(void* data, enum data_type data_type, int word_offset);
void evaluation_context_save(void* data, enum data_type data_type, int word_offset, int value);

static inline int evaluation_context_stack_size(struct evaluation_context* context) {
    return context->current_stack;
}

#endif