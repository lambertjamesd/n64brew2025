#ifndef __RING_ALLOCATOR_H__
#define __RING_ALLOCATOR_H__

struct ring_allocator {
    char* buffer;
    void* next_free;
    void* last_free;
};

void ring_init(struct ring_allocator* allocator, int capacity);
void ring_init_with_buffer(struct ring_allocator* allocator, void* buffer, int buffer_bytes);
void ring_destroy(struct ring_allocator* allocator);

int ring_get_free_memory(struct ring_allocator* allocator);

void* ring_malloc(struct ring_allocator* allocator, int bytes);
void ring_free(struct ring_allocator* allocator, void* target);

#endif