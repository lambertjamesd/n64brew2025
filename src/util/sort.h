#ifndef __UTIL_SORT_H__
#define __UTIL_SORT_H__

#include <stdint.h>

typedef int (*sort_compare)(void* data, uint16_t a, uint16_t b);

void sort_indices(uint16_t* array, int element_count, void* data, sort_compare compare);

#endif