#include "callback_list.h"

#include <malloc.h>
#include <memory.h>
#include <assert.h>
#include <stdbool.h>
#include "blist.h"

void callback_list_reset(struct callback_list* list, int data_size, int min_capcity, data_compare data_compare_callback) {
    free(list->elements);
    // malloc() pending_elements is done by realloc
    free(list->pending_elements);
    list->elements = NULL;
    memset(list, 0, sizeof(struct callback_list));
    // aligned to 4 bytes
    list->element_size = (sizeof(struct callback_element) + data_size + 3) & ~3;
    list->flags = 0;
    list->elements = malloc(list->element_size * min_capcity);
    list->capacity = min_capcity;
    list->data_compare_callback = data_compare_callback;
    list->pending_elements = 0;
    list->pending_element_count = 0;
    list->pending_element_capacity = 0;
}

int callback_list_element_compare(void* array, int a_index, int b_index) {
    struct callback_list* list = (struct callback_list*)array;
    struct callback_element* a = callback_list_get(list, a_index);
    struct callback_element* b = callback_list_get(list, b_index);

    if (list->data_compare_callback) {
        int compare_result = list->data_compare_callback(a + 1, b + 1);

        if (compare_result != 0) {
            return compare_result;
        }
    }

    return (int)a->callback - (int)b->callback;
}

void callback_list_insert_pending(struct callback_list* list, void* callback, int id, void* data) {
    if (list->pending_element_capacity == list->pending_element_count) {
        if (list->pending_element_capacity == 0) {
            list->pending_element_capacity = 8;
        } else {
            list->pending_element_capacity *= 2;
        }

        list->pending_elements = realloc(list->pending_elements, list->element_size * list->pending_element_capacity);
    }

    struct callback_element* next = (struct callback_element*)((char*)list->pending_elements + list->element_size * list->pending_element_count);
    next->id = id;
    next->callback = callback;
    memcpy(callback_element_get_data(next), data, list->element_size - sizeof(struct callback_element));

    list->pending_element_count += 1;
}

void callback_list_do_insert_with_id(struct callback_list* list, void* callback, int id, void* data) {
    if (list->capacity == list->count) {
        list->capacity *= 2;
        list->elements = realloc(list->elements, list->element_size * list->capacity);
    }

    struct callback_element* element = callback_list_get(list, list->count);

    element->callback = callback;
    element->id = id;
    // copy data payload in
    memcpy(element + 1, data, list->element_size - sizeof(struct callback_element));

    int insert_index = blist_insertion_index(list, list->count, callback_list_element_compare);

    struct callback_element* new_pos = callback_list_get(list, insert_index);

    if (new_pos != element) {
        // shift exisiting elements
        memmove(callback_list_get(list, insert_index + 1), new_pos, (int)element - (int)new_pos);

        // reassign into new position
        element = callback_list_get(list, insert_index);
        element->callback = callback;
        element->id = id;
        memcpy(element + 1, data, list->element_size - sizeof(struct callback_element));
    }

    list->count += 1;
}

callback_id callback_list_insert(struct callback_list* list, void* callback, void* data) {
    list->next_id += 1;
    callback_id result = list->next_id;

    callback_list_insert_with_id(list, callback, data, result);

    return result;
}

void callback_list_insert_with_id(struct callback_list* list, void* callback, void* data, callback_id id) {
    assert(list->element_size);

    if (list->flags & CALLBACK_LIST_ENUMERATING) {
        callback_list_insert_pending(list, callback, id, data);
        return;
    }

    callback_list_do_insert_with_id(list, callback, id, data);
}

void callback_list_remove(struct callback_list* list, callback_id id) {
    struct callback_element* found_at = NULL;
    struct callback_element* element = list->elements;

    for (int i = 0; i < list->count; i += 1) {
        if (element->id == id) {
            found_at = element;
            break;
        }

        element = callback_list_next(list, element);
    }

    // check for callback in new pending callbacks
    if (!found_at && list->pending_element_count) {
        element = list->pending_elements;

        for (int i = 0; i < list->pending_element_count; i += 1) {
            if (element->id == id) {
                found_at = element;
                break;
            }

            element = callback_list_next(list, element);
        }

        assert(found_at);
        found_at->id = 0;

        return;
    }

    assert(found_at);

    if (list->flags & CALLBACK_LIST_ENUMERATING) {
        // defer removal to callback_list_end
        found_at->id = 0;
        list->flags |= CALLBACK_LIST_HAS_DELETIONS;
        return;
    }

    struct callback_element* array_end = callback_list_get(list, list->count);
    struct callback_element* next_element = callback_list_next(list, found_at);

    if (next_element < array_end) {
        memmove(found_at, next_element, (int)array_end - (int)next_element); 
    }

    list->count -= 1;
}

void callback_list_begin(struct callback_list* list) {
    list->flags |= CALLBACK_LIST_ENUMERATING;
}

void callback_list_end(struct callback_list* list) {
    list->flags &= ~CALLBACK_LIST_ENUMERATING;

    if (list->flags & CALLBACK_LIST_HAS_DELETIONS) {
        struct callback_element* write_target = callback_list_get(list, 0);
        struct callback_element* read_target = callback_list_get(list, 0);

        int new_count = 0;

        for (int read_index = 0; read_index < list->count; read_target = callback_list_next(list, read_target), read_index += 1) {
            if (read_target->id == 0) {
                continue;
            }

            if (write_target != read_target) {
                memcpy(write_target, read_target, list->element_size);
            }
            
            write_target = callback_list_next(list, write_target);
            new_count += 1;
        }

        list->count = new_count;

        list->flags &= ~CALLBACK_LIST_HAS_DELETIONS;
    }

    struct callback_element* pending = list->pending_elements;    

    for (int i = 0; i < list->pending_element_count; ++i) {
        // pending id may be 0 if the callback was removed after it was added
        // and before callback_list_end
        if (pending->id) {
            callback_list_do_insert_with_id(list, pending->callback, pending->id, callback_element_get_data(pending));
        }

        pending = callback_list_next(list, pending);
    }

    list->pending_element_count = 0;
}