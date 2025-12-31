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

#define BORDER_MARGIN       5
#define CORNER_SIZE         13
#define BORDER_IMAGE_SIZE   32

#define INSET_SIZE          (CORNER_SIZE - BORDER_MARGIN)

void menu_common_render_background(int x, int y, int w, int h) {
    material_apply(menu_border_material);

    rdpq_texture_rectangle(
        TILE0,
        x - BORDER_MARGIN, y - BORDER_MARGIN,
        x + INSET_SIZE, y + INSET_SIZE, 
        0, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + INSET_SIZE, y - BORDER_MARGIN,
        x + w - INSET_SIZE, y + INSET_SIZE, 
        CORNER_SIZE, 0,
        BORDER_IMAGE_SIZE - CORNER_SIZE, CORNER_SIZE
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w - INSET_SIZE, y - BORDER_MARGIN,
        x + w + BORDER_MARGIN, y + INSET_SIZE, 
        BORDER_IMAGE_SIZE - CORNER_SIZE, 0
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x - BORDER_MARGIN, y + INSET_SIZE,
        x + INSET_SIZE, y + h - INSET_SIZE, 
        0, CORNER_SIZE,
        CORNER_SIZE, BORDER_IMAGE_SIZE - CORNER_SIZE
    );

    rdpq_texture_rectangle(
        TILE0,
        x - BORDER_MARGIN, y + h - INSET_SIZE,
        x + INSET_SIZE, y + h + BORDER_MARGIN, 
        0, BORDER_IMAGE_SIZE - CORNER_SIZE
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + INSET_SIZE, y + h - INSET_SIZE,
        x + w - INSET_SIZE, y + h + BORDER_MARGIN, 
        CORNER_SIZE, BORDER_IMAGE_SIZE - CORNER_SIZE,
        BORDER_IMAGE_SIZE - CORNER_SIZE, BORDER_IMAGE_SIZE
    );

    rdpq_texture_rectangle(
        TILE0,
        x + w - INSET_SIZE, y + h - INSET_SIZE,
        x + w + BORDER_MARGIN, y + h + BORDER_MARGIN, 
        BORDER_IMAGE_SIZE - CORNER_SIZE, BORDER_IMAGE_SIZE - CORNER_SIZE
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + w - INSET_SIZE, y + INSET_SIZE,
        x + w + BORDER_MARGIN, y + h - INSET_SIZE, 
        BORDER_IMAGE_SIZE - CORNER_SIZE, CORNER_SIZE,
        BORDER_IMAGE_SIZE, BORDER_IMAGE_SIZE - CORNER_SIZE
    );

    rdpq_texture_rectangle_scaled(
        TILE0,
        x + INSET_SIZE, y + INSET_SIZE,
        x + w - INSET_SIZE, y + h - INSET_SIZE,
        CORNER_SIZE, CORNER_SIZE,
        BORDER_IMAGE_SIZE - CORNER_SIZE, BORDER_IMAGE_SIZE - CORNER_SIZE
    );
}