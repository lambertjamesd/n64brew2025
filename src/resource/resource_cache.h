#ifndef __RESOURCE_RESOURCE_CACHE_H__
#define __RESOURCE_RESOURCE_CACHE_H__

#include <stdbool.h>
#include <stdint.h>

struct resource_cache_entry {
    char* filename;
    void* resource;
    short reference_count;
    short filename_index;
    uint32_t filename_hash;
};

struct resource_cache {
    struct resource_cache_entry* entries;
    short next_entry_index;
    short entry_capacity;
    short entry_count;

    short* filename_index;
    short* resource_index;
};

void resource_cache_reset(struct resource_cache* cache);
struct resource_cache_entry* resource_cache_next(struct resource_cache* cache, struct resource_cache_entry* entry);
struct resource_cache_entry* resource_cache_use(struct resource_cache* cache, const char* filename);
bool resource_cache_free(struct resource_cache* cache, void* resource);

#endif