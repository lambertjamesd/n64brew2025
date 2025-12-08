#include "ring_allocator.h"

#include <malloc.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define FREE_BLOCK_ID           0xDEAD
#define ALLOCATED_BLOCK_ID      0xBEEF

struct chunk_header {
    uint16_t allocated;
    // not including header and footer
    uint16_t size;
};

struct free_block
{
    struct free_block* next;
    struct free_block* prev;
};

#define ALIGN_UP(number)    (((number) + 7) & ~7)
#define ALIGN_DOWN(number)      ((number) & ~7)

#define GET_HEADER_RAW(buffer, offset) (struct chunk_header*)((char*)(buffer) + offset)

#define GET_FREE_BLOCK(buffer, offset)  (struct free_block*)((char*)buffer + (offset))

#define MIN_BLOCK_SZE   16

#define HEADER_FOOTER_SIZE  (sizeof(struct chunk_header) * 2)

void ring_init_block(void* data_start, bool allocated, uint16_t data_size) {
    assert(ALIGN_UP((int)data_start) == (int)data_start);
    assert(ALIGN_UP(data_size) == data_size);
    struct chunk_header* header = GET_HEADER_RAW(data_start, -sizeof(struct chunk_header));
    header->allocated = allocated ? ALLOCATED_BLOCK_ID : FREE_BLOCK_ID;
    header->size = data_size;

    struct chunk_header* footer = GET_HEADER_RAW(data_start, data_size);
    footer->allocated = allocated ? ALLOCATED_BLOCK_ID : FREE_BLOCK_ID;
    footer->size = data_size;
}

void ring_init_with_buffer(struct ring_allocator* allocator, void* buffer, int buffer_bytes) {
    void* end = (void*)ALIGN_DOWN((int)((char*)buffer + buffer_bytes));
    void* actual_start = (void*)ALIGN_UP((int)((char*)buffer));

    allocator->buffer = actual_start;

    int memory_size = (int)end - (int)actual_start;
    int capacity = memory_size - ALIGN_UP(HEADER_FOOTER_SIZE) * 2;

    ring_init_block((char*)allocator->buffer + ALIGN_UP(HEADER_FOOTER_SIZE), false, ALIGN_UP(capacity));
    
    struct free_block* free_block = GET_FREE_BLOCK(allocator->buffer, ALIGN_UP(HEADER_FOOTER_SIZE));
    free_block->next = free_block;
    free_block->prev = free_block;
    allocator->next_free = free_block;
    allocator->last_free = free_block;

    struct chunk_header* start_footer = GET_HEADER_RAW(allocator->buffer, 0);
    start_footer->allocated = ALLOCATED_BLOCK_ID;
    start_footer->size = 0;

    struct chunk_header* end_header = GET_HEADER_RAW(allocator->buffer, memory_size - sizeof(struct chunk_header));
    end_header->allocated = ALLOCATED_BLOCK_ID;
    end_header->size = 0;
}

void ring_init(struct ring_allocator* allocator, int capacity) {
    if (capacity < MIN_BLOCK_SZE) {
        capacity = MIN_BLOCK_SZE;
    }

    int memory_size = ALIGN_UP(HEADER_FOOTER_SIZE) * 2 + ALIGN_UP(capacity);

    ring_init_with_buffer(allocator, malloc(memory_size), memory_size);
}

int ring_get_free_memory(struct ring_allocator* allocator) {
    int result = 0;

    struct chunk_header* current = GET_HEADER_RAW(allocator->buffer, sizeof(struct chunk_header));

    while (current->size != 0) {
        if (current->allocated == FREE_BLOCK_ID) {
            result += current->size;
        }
        current = (struct chunk_header*)((char*)current + current->size + ALIGN_UP(HEADER_FOOTER_SIZE));
    }

    return result;
}

bool ring_has_allocated_block(struct ring_allocator* allocator) {
    if (allocator->next_free != GET_FREE_BLOCK(allocator->buffer, ALIGN_UP(HEADER_FOOTER_SIZE))) {
        return true;
    }

    struct chunk_header* free_block = GET_HEADER_RAW(allocator->buffer, ALIGN_UP(HEADER_FOOTER_SIZE) - sizeof(struct chunk_header));
    struct chunk_header* after_block = GET_HEADER_RAW(allocator->buffer, ALIGN_UP(HEADER_FOOTER_SIZE) + free_block->size + sizeof(struct chunk_header));

    return after_block->size != 0;
}

void ring_destroy(struct ring_allocator* allocator) {
    assert(!ring_has_allocated_block(allocator));
    free(allocator->buffer);
}

void* ring_malloc_alloc_free(struct ring_allocator* allocator, struct chunk_header* free_header, int bytes) {
    void* result = free_header + 1;

    int remaining_bytes = free_header->size - bytes - ALIGN_UP(HEADER_FOOTER_SIZE);

    struct free_block* free_block = (struct free_block*)result;
    struct free_block* next_block = free_block->next;
    struct free_block* prev_block = free_block->prev;

    if (remaining_bytes < MIN_BLOCK_SZE) {
        bytes = free_header->size;

        if (free_block == next_block) {
            allocator->next_free = NULL;
            allocator->last_free = NULL;
        } else {
            prev_block->next = next_block;
            next_block->prev = prev_block;

            if (free_block == allocator->next_free) {
                allocator->next_free = next_block;
            }

            if (free_block == allocator->last_free) {
                allocator->last_free = next_block;
            }
        }
    } else {
        struct free_block* remaining_block = (struct free_block*)((char*)result + bytes + ALIGN_UP(HEADER_FOOTER_SIZE));
        ring_init_block(remaining_block, false, remaining_bytes);

        if (next_block == free_block) {
            remaining_block->next = remaining_block;
            remaining_block->prev = remaining_block;
        } else {
            next_block->prev = remaining_block;
            prev_block->next = remaining_block;

            remaining_block->prev = prev_block;
            remaining_block->next = next_block;
        }

        if (free_block == allocator->next_free) {
            allocator->next_free = remaining_block;
        }

        if (free_block == allocator->last_free) {
            allocator->last_free = remaining_block;
        }
    }

    ring_init_block(result, true, bytes);

    return result;
}

void* ring_malloc(struct ring_allocator* allocator, int bytes) {
    if (allocator->next_free == NULL) {
        return NULL;
    }

    bytes = ALIGN_UP(bytes);

    struct free_block* next_free = allocator->next_free;
    struct free_block* start_free = next_free;
    
    do {
        struct chunk_header* free_header = (struct chunk_header*)next_free - 1;

        if (bytes <= free_header->size) {
            return ring_malloc_alloc_free(allocator, free_header, bytes);
        }

        next_free = next_free->next;
    } while (next_free != start_free);

    return NULL;
}

void ring_free(struct ring_allocator* allocator, void* target) {
    if (!target) {
        return;
    }

    struct chunk_header* free_header = (struct chunk_header*)target - 1;
    assert(free_header->allocated == ALLOCATED_BLOCK_ID);
    assert(((struct chunk_header*)((char*)target + free_header->size))->allocated == ALLOCATED_BLOCK_ID);

    struct free_block* new_block = target;

    new_block->next = NULL;
    new_block->prev = NULL;
    
    struct chunk_header* prev_header = free_header - 1;
    struct chunk_header* next_header = (struct chunk_header*)((char*)new_block + free_header->size + sizeof(struct chunk_header));

    if (prev_header->allocated == FREE_BLOCK_ID) {
        // simply merge with previous block
        struct free_block* prev_block = (struct free_block*)((char*)prev_header - prev_header->size);
        ring_init_block(prev_block, false, prev_header->size + free_header->size + HEADER_FOOTER_SIZE);
        new_block = prev_block;
        free_header = (struct chunk_header*)prev_block - 1;
    }

    if (next_header->allocated == FREE_BLOCK_ID) {
        struct free_block* next_block = (struct free_block*)(next_header + 1);

        if (allocator->last_free == next_block) {
            allocator->last_free = next_block->prev == next_block ? NULL : next_block->prev;
        }

        if (allocator->next_free == next_block) {
            allocator->next_free = next_block->next == next_block ? NULL : next_block->next;
        }

        struct free_block* next_next = next_block->next;
        struct free_block* next_prev = next_block->prev;

        next_next->prev = next_prev;
        next_prev->next = next_next;

        ring_init_block(new_block, false, next_header->size + free_header->size + HEADER_FOOTER_SIZE);
    }

    if (allocator->next_free == NULL) {
        allocator->next_free = new_block;
        allocator->last_free = new_block;

        new_block->next = new_block;
        new_block->prev = new_block;
        
        ring_init_block(new_block, false, free_header->size);
        return;
    }

    if (new_block->next == NULL) {
        struct free_block* prev_last = (struct free_block*)allocator->last_free;

        new_block->next = prev_last->next;
        prev_last->next->prev = new_block;

        new_block->prev = prev_last;
        prev_last->next = new_block;

        allocator->last_free = new_block;

        ring_init_block(new_block, false, free_header->size);
    }
}