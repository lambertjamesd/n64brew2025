#include "blist.h"

#include <assert.h>
#include <stdbool.h>

int blist_insertion_index(void* array, int array_length, array_element_comparer comparer) {
    int max = array_length;
    int min = 0;
    int mid = (max + min) >> 1;

    if (array_length == 0) {
        return 0;
    }

    for (int i = 0; i < array_length || i < 4; ++i) {
        int compare_result = comparer(array, mid, array_length);

        if (compare_result <= 0) {
            if (min == mid) {
                return mid + 1;
            }

            min = mid;
        } else {
            if (max == mid) {
                return mid;
            }

            max = mid;
        }

        mid = (max + min) >> 1;
    }

    // this should never be reached
    assert(false);

    return mid;
}
