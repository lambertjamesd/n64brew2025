#include "memory_stream.h"

#include <memory.h>

void memory_stream_init(struct memory_stream* stream, const void* memory, int capacity) {
    stream->curr = memory;
    stream->end = stream->curr + capacity;
}

int memory_stream_read(struct memory_stream* stream, void* into, int amount) {
    int max = stream->end - stream->curr;

    if (amount > max) {
        amount = max;
    }

    if (amount <= 0) {
        return 0;
    }

    if (into) {
        memcpy(into, stream->curr, amount);
    }
    stream->curr += amount;

    return amount;
}

const void* memory_stream_curr(struct memory_stream* stream) {
    return stream->curr; 
}