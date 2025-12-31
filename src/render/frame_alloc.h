#ifndef __RENDER_FRAME_ALLOC_H__
#define __RENDER_FRAME_ALLOC_H__

#include <stdint.h>
#include <t3d/t3dmath.h>
#include "../math/transform.h"
#include "../math/transform_single_axis.h"

#define FRAME_MEMORY_SIZE   32 * 1024
#define FRAME_WORD_COUNT    (FRAME_MEMORY_SIZE / sizeof(uint64_t))

#define ALIGN_16(input)     (((input) + 15) & ~15)

struct frame_memory_pool {
    uint64_t memory[FRAME_WORD_COUNT];
    uint16_t current_word;
} __attribute__((aligned(16)));

typedef struct frame_memory_pool frame_memory_pool_t;

void frame_pool_reset(struct frame_memory_pool* pool);
void* frame_malloc(struct frame_memory_pool* pool, int bytes);

T3DMat4FP* frame_pool_get_transformfp(struct frame_memory_pool* pool);

#endif