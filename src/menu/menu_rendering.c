#include "menu_rendering.h"

#include <libdragon.h>
#include <GL/gl.h>
#include "../util/callback_list.h"

#define MIN_MENU_COUNT  8

static struct callback_list g_menu_callbacks;

struct menu_callback_data {
    void* data;
    int priority;
};

int menu_data_compare(void* a, void* b) {
    struct menu_callback_data* a_el = (struct menu_callback_data*)a;
    struct menu_callback_data* b_el = (struct menu_callback_data*)b;
    return a_el->priority - b_el->priority;
}

void menu_reset() {
    callback_list_reset(&g_menu_callbacks, sizeof(struct menu_callback_data), MIN_MENU_COUNT, menu_data_compare);
}

void menu_add_callback(menu_render_callback callback, void* data, enum menu_priority priority) {
    struct menu_callback_data entry;

    entry.data = data;
    entry.priority = priority;

    callback_list_insert_with_id(&g_menu_callbacks, callback, &entry, (callback_id)data);
}

void menu_remove_callback(void* data) {
    callback_list_remove(&g_menu_callbacks, (callback_id)data);
}

void menu_render() {
    rdpq_sync_pipe();
    rdpq_mode_zbuf(false, false);
    rdpq_mode_persp(false);

    struct callback_element* current = callback_list_get(&g_menu_callbacks, 0);

    for (int i = 0; i < g_menu_callbacks.count; ++i) {
        struct menu_callback_data* element = callback_element_get_data(current);

        ((menu_render_callback)current->callback)(element->data);
        
        current = callback_list_next(&g_menu_callbacks, current);
    }
}