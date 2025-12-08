#ifndef __RESOURCE_WAV_CACHE_H__
#define __RESOURCE_WAV_CACHE_H__

#include <libdragon.h>

wav64_t* wav_cache_load(const char* filename);
void wav_cache_release(wav64_t* wav);

#endif