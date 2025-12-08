#include "time.h"

#include <malloc.h>
#include <memory.h>
#include <stdio.h>

#include "../util/flags.h"
#include "../util/blist.h"
#include "../util/callback_list.h"

struct update_element {
    void* data;
    short priority;
    short mask;
};

struct update_state {
    struct callback_list callbacks;
    int enabled_layers;
};

#define MIN_UPDATE_COUNT    64

static struct update_state g_update_state;
float fixed_time_step;
float scaled_time_step;
float scaled_time_step_inv;
float total_time;
float game_time;
float global_time_scale = 1.0f;
float render_time_step;
float last_render_time = 0.0f;

int update_compare_elements(void* a, void* b) {
    struct update_element* a_el = (struct update_element*)a;
    struct update_element* b_el = (struct update_element*)b;
    return a_el->priority - b_el->priority;
}

void update_reset() {
    callback_list_reset(&g_update_state.callbacks, sizeof(struct update_element), MIN_UPDATE_COUNT, update_compare_elements);
    g_update_state.enabled_layers = ~0;
    fixed_time_step = DEFAULT_TIME_STEP;
}

void update_add(void* data, update_callback callback, int priority, int mask) {
    struct update_element element;

    element.data = data;
    element.priority = priority;
    element.mask = mask;

    callback_list_insert_with_id(&g_update_state.callbacks, callback, &element, (callback_id)data);
}

void update_remove(void* data) {
    callback_list_remove(&g_update_state.callbacks, (callback_id)data);
}

void update_set_layers(void* data, int mask) {
    struct callback_element* current = callback_list_get(&g_update_state.callbacks, 0);

    for (int i = 0; i < g_update_state.callbacks.count; ++i) {
        struct update_element* element = callback_element_get_data(current);

        if (element->data == data) {
            element->mask = mask;
            return;
        }
        
        current = callback_list_next(&g_update_state.callbacks, current);
    }
}

void update_pause_layers(int mask) {
    CLEAR_FLAG(g_update_state.enabled_layers, mask);
}

void update_unpause_layers(int mask) {
    SET_FLAG(g_update_state.enabled_layers, mask);
}

bool update_has_layer(int mask) {
    return mask & g_update_state.enabled_layers;
}

void update_render_time() {
    render_time_step = game_time - last_render_time;
    last_render_time = game_time;
}

void update_dispatch() {
    total_time += fixed_time_step;
    scaled_time_step = fixed_time_step * global_time_scale;
    scaled_time_step_inv = 1.0f / scaled_time_step;

    if (g_update_state.enabled_layers & UPDATE_LAYER_WORLD) {
        game_time += scaled_time_step;
    }

    callback_list_begin(&g_update_state.callbacks);

    struct callback_element* current = callback_list_get(&g_update_state.callbacks, 0);

    for (int i = 0; i < g_update_state.callbacks.count; ++i) {
        struct update_element* element = callback_element_get_data(current);

        if ((element->mask & g_update_state.enabled_layers) != 0 && current->id) {
            ((update_callback)current->callback)(element->data);
        }
        
        current = callback_list_next(&g_update_state.callbacks, current);
    }

    callback_list_end(&g_update_state.callbacks);
}