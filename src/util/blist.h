#ifndef __UTIL_BLIST_H__
#define __UTIL_BLIST_H__

// should return array[a_index] - array[b_index]
typedef int (*array_element_comparer)(void* array, int a_index, int b_index);

// the new element should be at the index of array_length before calling
int blist_insertion_index(void* array, int array_length, array_element_comparer comparer);

#endif