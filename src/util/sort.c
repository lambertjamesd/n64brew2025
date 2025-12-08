#include "sort.h"

void sort_array_recurse(uint16_t* array, uint16_t* tmp, int start, int end, void* data, sort_compare compare) {
    if (start + 1 >= end) {
        return;
    }

    int mid = (start + end) >> 1;

    sort_array_recurse(array, tmp, start, mid, data, compare);
    sort_array_recurse(array, tmp, mid, end, data, compare);

    int a = start;
    int b = mid;
    int output = start;

    while (a < mid || b < end) {
        uint16_t a_index = array[a];
        uint16_t b_index = array[b];

        if (b >= end || (a < mid && compare(data, a_index, b_index) < 0)) {
            tmp[output] = array[a];
            ++output;
            ++a;
        } else {
            tmp[output] = array[b];
            ++output;
            ++b;
        }
    }

    for (int i = start; i < end; ++i) {
        array[i] = tmp[i];
    }
}

void sort_indices(uint16_t* array, int element_count, void* data, sort_compare compare) {
    uint16_t tmp[element_count];
    sort_array_recurse(array, tmp, 0, element_count, data, compare);
}