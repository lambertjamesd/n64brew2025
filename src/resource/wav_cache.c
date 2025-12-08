#include "wav_cache.h"

#include "resource_cache.h"

static struct resource_cache wav_resource_cache;

wav64_t* wav_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&wav_resource_cache, filename);

    if (entry->resource == NULL) {
        entry->resource = wav64_load(filename, NULL);
    }

    return entry->resource;
}

void wav_cache_release(wav64_t* wav) {
    if (resource_cache_free(&wav_resource_cache, wav)) {
        wav64_close(wav);
    }
}