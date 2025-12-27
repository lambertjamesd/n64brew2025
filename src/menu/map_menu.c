#include "map_menu.h"

#include <libdragon.h>
#include "../resource/material_cache.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../menu/menu_rendering.h"
#include "../math/vector2s16.h"
#include "../scene/scene.h"
#include "../math/minmax.h"

struct map_asssets {
    sprite_t* map;
    material_t* material;
};

struct map_menu {
    vector2s16_t last_position;
    bool can_unpause;
};

#define MAP_TILE_SIZE           32
#define MAP_SIZE                128

#define MAP_X                   30
#define MAP_Y                   56

#define BRUSH_HALF_SIZE         6
#define BLUR_RADIUS             2

static uint8_t __attribute__((aligned(16))) map_revealed[MAP_SIZE * MAP_SIZE];
static uint8_t reveal_brush[BRUSH_HALF_SIZE * BRUSH_HALF_SIZE];
static struct map_asssets assets;
static struct map_menu map_menu;

void map_render(void* data) {
    surface_t surf = sprite_get_pixels(assets.map);

    surf.width = MAP_TILE_SIZE;
    surf.height = MAP_TILE_SIZE;

    rdpq_texparms_t tex_params = {
        .palette = 0,
        .tmem_addr = 0,
    };
    
    surface_t mask_surface = {
        .buffer = map_revealed,
        .flags = 0x11,
        .stride = 0x40,
        .width = MAP_TILE_SIZE,
        .height = MAP_TILE_SIZE,
    };
    rdpq_texparms_t mask_parms = {
        .palette = 0,
        .tmem_addr = 2048,
    };

    rdpq_sync_pipe();
    rspq_block_run(assets.material->block);

    for (int y = 0; y < MAP_SIZE; y += MAP_TILE_SIZE) {
        for (int x = 0; x < MAP_SIZE; x += MAP_TILE_SIZE) {
            int pixel_index = x + y * MAP_SIZE;

            surf.buffer = (void*)((uint16_t*)assets.map->data + pixel_index);
            mask_surface.buffer = (void*)(map_revealed + pixel_index);


            rdpq_tex_upload(TILE1, &mask_surface, &mask_parms);
            rdpq_tex_upload(TILE0, &surf, &tex_params);

            rdpq_texture_rectangle(
                TILE0, 
                MAP_X + x, MAP_Y + y, 
                MAP_X + x + MAP_TILE_SIZE, MAP_Y + y + MAP_TILE_SIZE, 
                0, 0
            );
        }
    }
}

void map_menu_init() {
    for (int y = 0; y < MAP_SIZE; y += 1) {
        for (int x = 0; x < MAP_SIZE; x += 1) {
            map_revealed[x + y * MAP_SIZE] = 32;
        }
    }
    data_cache_hit_writeback_invalidate(map_revealed, sizeof(map_revealed));

    uint8_t* pixel = reveal_brush;

    for (int y = 0; y < BRUSH_HALF_SIZE; y += 1) {
        for (int x = 0; x < BRUSH_HALF_SIZE; x += 1) {
            float distance = sqrtf(x * x + y * y);
            if (distance < BRUSH_HALF_SIZE - BLUR_RADIUS) {
                *pixel = 255;
            } else if (distance > BRUSH_HALF_SIZE) {
                *pixel = 0;
            } else {
                *pixel = (uint8_t)(255.0f * (distance - (BRUSH_HALF_SIZE - BLUR_RADIUS)) * (1.0f / BLUR_RADIUS));
            }

            pixel += 1;
        }
    }
}

void map_menu_destroy() {

}

void map_menu_update(void* data) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);
    joypad_inputs_t input = joypad_get_inputs(0);

    if (!input.btn.start) {
        map_menu.can_unpause = true;
    }
    
    if (pressed.start && map_menu.can_unpause) {
        map_menu_hide();
        map_menu.can_unpause = false;
    }
}

void map_menu_show() {
    if (!current_scene || !current_scene->overworld) {
        return;
    }

    assets.map = sprite_load("rom:/images/menu/map.sprite");
    assets.material = material_cache_load("rom:/materials/menu/map.mat");
    update_pause_layers(UPDATE_LAYER_WORLD);
    update_unpause_layers(UPDATE_LAYER_PAUSE_MENU);
    game_mode_enter_menu();
    menu_add_callback(map_render, &map_menu, MENU_PRIORITY_OVERLAY);
    update_add(&map_menu, map_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_PAUSE_MENU);
    map_menu.can_unpause = false;
}

void map_menu_hide() {
    sprite_free(assets.map);
    material_cache_release(assets.material);
    assets.map = NULL;
    assets.material = NULL;
    update_unpause_layers(UPDATE_LAYER_WORLD);
    update_pause_layers(UPDATE_LAYER_PAUSE_MENU);
    game_mode_exit_menu();
    menu_remove_callback(&map_menu);
    update_remove(&map_menu);
}

void map_mark_revealed(struct Vector3* pos) {
    if (!current_scene || !current_scene->overworld) {
        return;
    }

    overworld_t* overworld = current_scene->overworld;    

    int center_x = (int)(MAP_SIZE * (pos->x - overworld->min.x) * overworld->inv_tile_size / overworld->tile_x);
    int center_y = (int)(MAP_SIZE * (pos->z - overworld->min.y) * overworld->inv_tile_size / overworld->tile_x);

    if (center_x == map_menu.last_position.x && center_y == map_menu.last_position.y) {
        return;
    }

    map_menu.last_position.x = center_x;
    map_menu.last_position.y = center_y;

    for (int dy = -BRUSH_HALF_SIZE + 1; dy < BRUSH_HALF_SIZE; dy += 1) {
        int y = center_y + dy;

        if (y < 0 || y >= MAP_SIZE) {
            continue;
        }

        uint8_t* target_row = &map_revealed[y * MAP_SIZE];
        uint8_t* src_row = &reveal_brush[abs(dy) * BRUSH_HALF_SIZE];

        for (int dx = -BRUSH_HALF_SIZE + 1; dx < BRUSH_HALF_SIZE; dx += 1) {
            int x = center_x + dx;

            if (x < 0 || x >= MAP_SIZE) {
                continue;
            }

            uint8_t* pixel = &target_row[x];

            *pixel = MAX(*pixel, src_row[abs(dx)]);
        }

        uint8_t* chunk_a = (uint8_t*)ALIGN_16((int)target_row + center_x - BRUSH_HALF_SIZE + 1);
        uint8_t* chunk_b = (uint8_t*)ALIGN_16((int)target_row + center_x + BRUSH_HALF_SIZE - 1);

        data_cache_hit_writeback(chunk_a, 16);
        if (chunk_a != chunk_b) {
            data_cache_hit_writeback(chunk_b, 16);
        }
    }
}