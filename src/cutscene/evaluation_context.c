#include "evaluation_context.h"

#include <assert.h>
#include <malloc.h>
#include <memory.h>

void evaluation_context_init(struct evaluation_context* context, int locals_size) {
    context->current_stack = 0;
    context->local_varaibles = locals_size ? malloc(locals_size) : NULL;
}

void evaluation_context_destroy(struct evaluation_context* context) {
    free(context->local_varaibles);
}

void evaluation_context_push(struct evaluation_context* context, int value) {
    assert(context->current_stack < MAX_STACK_SIZE);
    context->stack[context->current_stack] = value;
    ++context->current_stack;
}

int evaluation_context_pop(struct evaluation_context* context) {
    assert(context->current_stack > 0);
    --context->current_stack;
    return context->stack[context->current_stack];
}

int evaluation_context_peek(struct evaluation_context* context) {
    return context->stack[context->current_stack];
}

void evaluation_context_popn(struct evaluation_context* context, int* into, int count) {
    assert(context->current_stack >= count);
    context->current_stack -= count;
    if (into) {
        memcpy(into, &context->stack[context->current_stack], sizeof(int) * count);
    }
}

int evaluation_context_load(void* data, enum data_type data_type, int word_offset) {
    switch (data_type) {
        case DATA_TYPE_NULL:
            return 0;
        case DATA_TYPE_S8:
            return ((int8_t*)data)[word_offset];
        case DATA_TYPE_S16:
            return ((int16_t*)data)[word_offset];
        case DATA_TYPE_S32:
        case DATA_TYPE_F32:
            return ((int32_t*)data)[word_offset];
        case DATA_TYPE_BOOL: {
            uint32_t word = ((uint32_t*)data)[word_offset >> 5];
            uint32_t mask = (uint32_t)0x80000000 >> (word_offset & 0x1F);
            return (mask & word) != 0;
        }
        case DATA_TYPE_ADDRESS:
            return (int)((char*)data + word_offset);
        default:
            return 0;
    }
}

void evaluation_context_save(void* data, enum data_type data_type, int word_offset, int value) {
    switch (data_type) {
        case DATA_TYPE_NULL:
            break;
        case DATA_TYPE_S8:
            ((int8_t*)data)[word_offset] = value;
            break;
        case DATA_TYPE_S16:
            ((int16_t*)data)[word_offset] = value;
            break;
        case DATA_TYPE_S32:
        case DATA_TYPE_F32:
            ((int32_t*)data)[word_offset] = value;
            break;
        case DATA_TYPE_BOOL: {
            uint32_t* word = &((uint32_t*)data)[word_offset >> 5];
            uint32_t mask = ((uint32_t)0x80000000 >> (word_offset & 0x1F));
            *word = (*word & ~mask) | (value ? mask : 0);
            break;
        }
        default:
            break;
    }
}