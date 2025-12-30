#include "fade_effect.h"

#include <stdbool.h>
#include "../time/time.h"
#include "../menu/menu_rendering.h"
#include "../render/defs.h"
#include "../resource/material_cache.h"

static color_t start_color;
static color_t end_color;
static float lerp_time;
static float current_time;
static bool is_active;

static color_t flash_color;
static material_t* solid_primitive_material;

color_t fade_effect_calculate_color() {
    if (flash_color.a != 0) {
        color_t result = flash_color;
        flash_color.a = 0;
        return result;
    }

    if (current_time <= 0.0f && lerp_time > 0.0f) {
        return start_color;
    }

    if (current_time >= lerp_time) {
        return end_color;
    }

    return coloru8_lerp(&start_color, &end_color, current_time / lerp_time);
}

void fade_effect_update(void* data) {
    if (current_time < lerp_time) {
        current_time += fixed_time_step;
    }
}

void fade_effect_render(void* data) {
    rspq_block_run(solid_primitive_material->block);

    color_t color = fade_effect_calculate_color();

    if (color.a == 0) {
        return;
    }

    rdpq_set_prim_color((color_t){color.r, color.g, color.b, color.a});
    rdpq_texture_rectangle(
        TILE0, 
        0, 0,
        320, 240,
        1, 1
    );
}

void fade_effect_activate() {
    if (is_active) {
        return;
    }
    is_active = true;

    update_add(&start_color, fade_effect_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_WORLD | UPDATE_LAYER_PAUSE_MENU);
    menu_add_callback(fade_effect_render, &start_color, MENU_PRIORITY_OVERLAY);
}

void fade_effect_init() {
    solid_primitive_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void fade_effect_set(color_t color, float time) {
    start_color = fade_effect_calculate_color();
    if (start_color.a == 0) {
        start_color = color;
        start_color.a = 0;
    }
    if (color.a == 0) {
        end_color = start_color;
        end_color.a = 0;
    } else {
        end_color = color;
    }
    lerp_time = time;
    current_time = 0.0f;

    fade_effect_activate();
}

void fade_effect_flash(color_t color) {
    flash_color = color;
    fade_effect_activate();
}