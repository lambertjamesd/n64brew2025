#ifndef __RESOURCE_ANIMATION_CACHE_H__
#define __RESOURCE_ANIMATION_CACHE_H__

#include "../render/animation_clip.h"

struct animation_set* animation_cache_load(const char* filename);
void animation_cache_release(struct animation_set* animations);

#endif