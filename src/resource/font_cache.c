#include "font_cache.h"

#include "resource_cache.h"

static struct resource_cache font_resource_cache;

rdpq_font_t* font_cache_load(char* filename) {
    struct resource_cache_entry* entry = resource_cache_use(&font_resource_cache, filename);

    if (!entry->resource) {
        entry->resource = rdpq_font_load(filename);
    }

    return entry->resource;
}

void font_cache_release(rdpq_font_t* font) {
    if (resource_cache_free(&font_resource_cache, font)) {
        rdpq_font_free(font);
    }
}