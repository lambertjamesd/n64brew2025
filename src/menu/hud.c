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

void hud_render(void *data) {
    hud_render_interaction_preview(data);
}

void hud_init(struct hud* hud, struct player* player, camera_t* camera) {
    menu_add_callback(hud_render, hud, MENU_PRIORITY_HUD);
    font_type_use(FONT_DIALOG);
    hud->player = player;
    hud->camera = camera;

    hud->assets.overlay_material = material_cache_load("rom:/materials/menu/solid_primitive.mat");
}

void hud_destroy(struct hud* hud) {
    menu_remove_callback(hud);
    font_type_release(FONT_DIALOG);
    material_cache_release(hud->assets.overlay_material);
}