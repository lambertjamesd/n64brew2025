#include "animation_cache.h"

#include "resource_cache.h"

static struct resource_cache animation_resource_cache;

struct animation_set* animation_cache_load(const char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&animation_resource_cache, filename);

    if (!entry->resource) {
        entry->resource = animation_set_load(filename);
    }

    return entry->resource;
}
void animation_cache_release(struct animation_set* animations) {
    if (resource_cache_free(&animation_resource_cache, animations)) {
        annotation_clip_set_free(animations);
    }
}