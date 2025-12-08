#ifndef __UTIL_CALLBACK_LIST_H__
#define __UTIL_CALLBACK_LIST_H__

typedef int callback_id;

typedef int (*data_compare)(void* a, void* b);

struct callback_element {
    callback_id id;
    void* callback;
};

enum callback_list_flags {
    CALLBACK_LIST_ENUMERATING = (1 << 0),
    CALLBACK_LIST_HAS_DELETIONS = (1 << 1),
};

struct callback_list {
    callback_id next_id;
    void* elements;
    short count;
    short capacity;
    short element_size;

    short flags;

    void* pending_elements;
    short pending_element_count;
    short pending_element_capacity;

    data_compare data_compare_callback;
};

void callback_list_reset(struct callback_list* list, int data_size, int min_capcity, data_compare data_compare_callback);

#define callback_list_get(list, index) (struct callback_element*)((char*)(list)->elements + (index) * (list)->element_size)
#define callback_element_get_data(element) (void*)((element) + 1)
#define callback_list_next(list, curr) (struct callback_element*)((char*)(curr) + (list)->element_size)

callback_id callback_list_insert(struct callback_list* list, void* callback, void* data);
void callback_list_insert_with_id(struct callback_list* list, void* callback, void* data, callback_id id);
void callback_list_remove(struct callback_list* list, callback_id id);

void callback_list_begin(struct callback_list* list);
void callback_list_end(struct callback_list* list);

#endif