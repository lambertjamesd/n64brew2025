#include "menu_common.h"

#include "../resource/material_cache.h"

static struct material* menu_background_material;
static struct material* menu_border_material;

struct material* menu_icons_material;
struct material* solid_primitive_material;
struct material* sprite_blit;

void menu_common_init() {
    // material_cache_release() never called
    menu_background_material = material_cache_load("rom:/materials/menu/menu_corner.mat");
    // material_cache_release() never called
    menu_border_material = material_cache_load("rom:/materials/menu/menu_border.mat");
    // material_cache_release() never called
    menu_icons_material = material_cache_load("rom:/materials/menu/menu_icons.mat");
    // material_cache_release() never called
    solid_primitive_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
    // material_cache_release() never called
    sprite_blit = material_cache_load("rom:/materials/menu/sprite_blit.mat");
}

void menu_common_render_background(int x, int y, int w, int h) {
    rdpq_sync_pipe();
    rspq_block_run(menu_border_material->block);

    rdpq_texture_rectangle(
        TILE0,
        x - 3, y - 3,
        x, y, 
        0, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y - 3,
        x + w, y, 
        3, 0,
        6, 3
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w, y - 3,
        x + w + 3, y, 
        6, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x - 3, y,
        x, y + h, 
        0, 3,
        3, 6
    );

    rdpq_texture_rectangle(
        TILE0,
        x - 3, y + h,
        x, y + h + 3, 
        0, 6
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y + h,
        x + w, y + h + 3, 
        3, 6,
        6, 9
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w, y + h,
        x + w + 3, y + h + 3, 
        6, 6
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + w, y,
        x + w + 3, y + h, 
        6, 3,
        9, 6
    );

    rdpq_sync_pipe();
    rspq_block_run(menu_background_material->block);

    rdpq_texture_rectangle_scaled(
        TILE0,
        x, y,
        x + w, y + h,
        0, 0,
        64, 64
    );
}