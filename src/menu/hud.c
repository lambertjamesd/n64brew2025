#include "hud.h"

#include "menu_rendering.h"
#include "../resource/material_cache.h"
#include "../cutscene/cutscene_runner.h"
#include "../render/coloru8.h"
#include "../resource/material_cache.h"
#include "../collision/collision_scene.h"
#include "../render/screen_coords.h"
#include "../time/time.h"
#include "../render/defs.h"
#include "../fonts/fonts.h"
#include "../resource/material_cache.h"
#include "../scene/scene.h"
#include "../math/vector2.h"
#include "menu_common.h"
#include <string.h>

#define SCREEN_EDGE_MARGIN      20.0f
#define TEXT_PADDING            2
#define BOX_HEIGHT              10

static sprite_t* map_test;
static material_t* map_render;

int measure_text(enum font_type font, const char* message) {
    const char* curr = message;

    int result = 0;

    while (*curr) {
        rdpq_font_gmetrics_t metrics;
        bool was_found = rdpq_font_get_glyph_metrics(font_get(font), *curr, &metrics);
        ++curr;

        if (was_found) {
            result += metrics.xadvance;
        }
    }
    
    return result;
}

void hud_render_interaction_preview(struct hud* hud) {
    if (!hud->player->hover_interaction || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    interactable_t *target = interactable_get(hud->player->hover_interaction);

    if (!target) {
        return;
    }

    const char* interaction_name = interact_type_to_name(target->interact_type);

    if (!interaction_name) {
        return;
    }

    dynamic_object_t *obj = collision_scene_find_object(hud->player->hover_interaction);

    if (!obj) {
        return;
    }

    vector2_t screen_pos;
    vector3_t pos = *obj->position;
    pos.y = obj->bounding_box.max.y;
    camera_screen_from_position(hud->camera, &pos, &screen_pos);

    if (screen_pos.x < -SCREEN_EDGE_MARGIN || screen_pos.y < -SCREEN_EDGE_MARGIN ||
        screen_pos.x > SCREEN_WD + SCREEN_EDGE_MARGIN || screen_pos.y > SCREEN_HT + SCREEN_EDGE_MARGIN) {
        return;
    }

    screen_pos.x = floorf(screen_pos.x);
    screen_pos.y = floorf(screen_pos.y);

    int box_width = measure_text(FONT_DIALOG, interaction_name);

    rdpq_sync_pipe();
    material_apply(hud->assets.overlay_material);
    rdpq_set_prim_color((color_t){0, 0, 0, 128});
    rdpq_texture_rectangle(
        TILE0, 
        screen_pos.x - TEXT_PADDING, screen_pos.y - BOX_HEIGHT - TEXT_PADDING, 
        screen_pos.x + box_width + TEXT_PADDING, screen_pos.y + TEXT_PADDING,
        0, 0
    );
    rdpq_sync_pipe();

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_BOTTOM,
            .width = 260,
            .height = 0,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        screen_pos.x, screen_pos.y, 
        interaction_name,
        strlen(interaction_name)
    );
}

static color_t compass_color = {0x5d, 0x56, 0x9f, 0xff};

static vector2_t arrow_vertices[] = {
    {0.0f, 10.0f},
    {-4.0f, -7.0f},
    {0.0f, -4.0f},
    {4.0f, -7.0f},
};

static vector2_t compass_center = {284.0f, 39.0f};

void hud_render_compass(struct hud* hud) {
    if (!current_scene || !current_scene->overworld) {
        return;
    }
    material_apply(hud->assets.icon_material);
    rdpq_sprite_blit(hud->assets.compass_border, SCREEN_WD - 20 - 32, 18, NULL);
    material_apply(hud->assets.compass_arrow);
    rdpq_set_prim_color(compass_color);

    vector2_t transformed[4];

    vector2_t* rot = player_get_rotation(&current_scene->player);
    for (int i = 0; i < 4; i += 1) {
        menu_transform_point(&arrow_vertices[i], rot, &compass_center, &transformed[i]);
    }
    rdpq_triangle(
        &TRIFMT_FILL, 
        (float*)&transformed[0], 
        (float*)&transformed[1], 
        (float*)&transformed[2]
    );
    rdpq_triangle(
        &TRIFMT_FILL, 
        (float*)&transformed[0], 
        (float*)&transformed[2], 
        (float*)&transformed[3]
    );
}

void hud_render(void *data) {
    hud_render_interaction_preview(data);
    hud_render_compass(data);
}

void hud_init(struct hud* hud, struct player* player, camera_t* camera) {
    menu_add_callback(hud_render, hud, MENU_PRIORITY_HUD);
    font_type_use(FONT_DIALOG);
    hud->player = player;
    hud->camera = camera;

    hud->assets.overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
    hud->assets.compass_border = sprite_load("rom:/images/menu/compass_border.sprite");
    hud->assets.icon_material = material_cache_load("rom:/materials/menu/map_icon.mat");
    hud->assets.compass_arrow = material_cache_load("rom:/materials/menu/map_arrow.mat");
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
    font_type_release(FONT_DIALOG);
    material_cache_release(hud->assets.overlay_material);
    material_cache_release(hud->assets.icon_material);
    material_cache_release(hud->assets.compass_arrow);
    sprite_free(hud->assets.compass_border);
}