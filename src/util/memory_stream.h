#ifndef __UTIL_MEMORY_STREAM_H__
#define __UTIL_MEMORY_STREAM_H__

struct memory_stream {
    const char* curr;
    const char* end;
};

typedef struct memory_stream memory_stream_t;

void memory_stream_init(struct memory_stream* stream, const void* memory, int capacity);
int memory_stream_read(struct memory_stream* stream, void* into, int amount);
const void* memory_stream_curr(struct memory_stream* stream);

#endif