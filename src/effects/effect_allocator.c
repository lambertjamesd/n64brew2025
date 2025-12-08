#include "effect_allocator.h"

#include "../util/ring_allocator.h"
#include <stdint.h>
#include <assert.h>

#define EFFECT_MEMORY_SIZE  0x8000

static uint64_t g_effect_buffer[EFFECT_MEMORY_SIZE / sizeof(uint64_t)];
static struct ring_allocator g_effect_allocator;

void* effect_malloc(int bytes) {
    if (!g_effect_allocator.buffer) {
        ring_init_with_buffer(&g_effect_allocator, &g_effect_buffer, EFFECT_MEMORY_SIZE);
    }

    return ring_malloc(&g_effect_allocator, bytes);
}

void effect_free(void* memory) {
    assert(g_effect_allocator.buffer);
    ring_free(&g_effect_allocator, memory);
}