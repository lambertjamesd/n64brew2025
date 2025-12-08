#include "sprite_cache.h"

#include "resource_cache.h"

struct resource_cache sprite_resource_cache;

sprite_t* sprite_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&sprite_resource_cache, filename);

    if (entry->resource == NULL) {
        entry->resource = sprite_load(filename);
    }

    return entry->resource;
}

void sprite_cache_release(sprite_t* sprite) {
    if (resource_cache_free(&sprite_resource_cache, sprite)) {
        sprite_free(sprite);
    }
}