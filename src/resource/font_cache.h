#ifndef __RESOURCE_FONT_CACHE_H__
#define __RESOURCE_FONT_CACHE_H__

#include <libdragon.h>

rdpq_font_t* font_cache_load(char* filename);
void font_cache_release(rdpq_font_t* font);

#endif