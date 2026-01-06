#include "map_menu.h"

#include <libdragon.h>
#include "../resource/material_cache.h"
#include "../time/time.h"
#include "../time/game_mode.h"
#include "../menu/menu_rendering.h"
#include "../math/vector2s16.h"
#include "../scene/scene.h"
#include "../math/minmax.h"
#include "../math/vector4.h"
#include "../fonts/fonts.h"
#include "menu_common.h"
#include "../player/inventory.h"
#include "../render/defs.h"
#include "../render/coloru8.h"

#define NEW_ITEM_ANIM_TIME      1.0f
#define OPEN_ITEM_DELAY_TIME    1.0f

enum menu_item_type {
    MENU_ITEM_PART,
    MENU_ITEM_MAP,

    MENU_ITEM_TYPE_COUNT,
};

struct map_asssets {
    sprite_t* map;
    material_t* material;
    material_t* map_background;
    material_t* map_arrow;
    material_t* map_view;
    material_t* map_icon;
    material_t* selection_cursor;

    sprite_t* icons[MENU_ITEM_TYPE_COUNT];
};

union menu_item_data {
    struct {
        const char* description;
    } part;
    struct {
        const char* image_filename;
    } map;
};

struct menu_item {
    enum menu_item_type type;
    enum inventory_item_type inventory_item;
    enum inventory_item_type hide_override;
    const char* name;
    union menu_item_data data;
};

static struct menu_item menu_items[] = {
    {
        .type = MENU_ITEM_MAP,
        .inventory_item = ITEM_WELL_PUMP_PART_MAP,
        .name = "Well part map",
        .data = {
            .map = {
                .image_filename = "rom:/images/maps/well_parts_map.sprite",
            },
        },
    }
};

#define MENU_ITEM_COUNT      (sizeof(menu_items) / sizeof(*menu_items))

enum map_menu_state {
    MAP_MENU_LIST,
    MAP_MENU_NEW_ITEMS,
    MAP_MENU_NEW_ITEM_DETAILS,
    MAP_MENU_DETAILS_ANIMATE,
    MAP_MENU_DETAILS,
};

union map_menu_state_data {
    struct {
        float timer;
    } new_items;
};

struct map_menu {
    enum map_menu_state state;
    union map_menu_state_data state_data;
    enum inventory_item_type selected_item;
    vector2s16_t last_position;
    bool can_unpause;

    bool has_prev[MENU_ITEM_COUNT];

    sprite_t* details_image;
};

#define MAP_TILE_SIZE           32
#define MAP_SIZE                128

#define MAP_X                   30
#define MAP_Y                   46

#define MENU_X                  (SCREEN_WD - MAP_X - MAP_SIZE)

#define BRUSH_HALF_SIZE         6
#define BLUR_RADIUS             2

#define ICON_SIZE               32

static uint8_t __attribute__((aligned(16))) map_revealed[MAP_SIZE * MAP_SIZE];
static uint8_t reveal_brush[BRUSH_HALF_SIZE * BRUSH_HALF_SIZE];
static struct map_asssets assets;
static struct map_menu map_menu;

static const char* icon_files[MENU_ITEM_TYPE_COUNT] = {
    [MENU_ITEM_PART] = "rom:/images/maps/dot_matrix_map_icon.sprite",
    [MENU_ITEM_MAP] = "rom:/images/maps/dot_matrix_map_icon.sprite",
};

static vector2_t player_cursor_points[3] = {
    {0.0f, 3.0f},
    {2.0f, -3.0f},
    {-2.0f, -3.0f},
};

#define TAN_HORZ 0.93361004861225457857f
#define VIEW_DEPTH  10.0f

static vector2_t camera_cursor_points[3] = {
    {0.0f, 0.0f},
    {-TAN_HORZ * VIEW_DEPTH, VIEW_DEPTH},
    {TAN_HORZ * VIEW_DEPTH, VIEW_DEPTH},
};

void map_get_position(vector3_t* world_pos, vector2_t* map_pos) {
    float width = current_scene->minimap_max.x - current_scene->minimap_min.x;
    float height = current_scene->minimap_max.y - current_scene->minimap_min.y;

    map_pos->x = (MAP_SIZE * (world_pos->x - current_scene->minimap_min.x) / width);
    map_pos->y = (MAP_SIZE * (1.0f - (world_pos->z - current_scene->minimap_min.y) / height));

}

void map_render_minimap(int map_x, int map_y) {
    if (!current_scene || !current_scene->overworld) {
        return;
    }

    material_apply(assets.map_background);

    rdpq_texture_rectangle(
        TILE0,
        map_x, map_y,
        map_x + MAP_SIZE,
        map_y + MAP_SIZE,
        13, 13
    );

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
        .stride = 0x80,
        .width = MAP_TILE_SIZE,
        .height = MAP_TILE_SIZE,
    };
    rdpq_texparms_t mask_parms = {
        .palette = 0,
        .tmem_addr = 2048,
    };

    material_apply(assets.material);

    for (int y = 0; y < MAP_SIZE; y += MAP_TILE_SIZE) {
        for (int x = 0; x < MAP_SIZE; x += MAP_TILE_SIZE) {
            int pixel_index = x + y * MAP_SIZE;

            surf.buffer = (void*)((uint16_t*)assets.map->data + pixel_index);
            mask_surface.buffer = (void*)(map_revealed + pixel_index);

            rdpq_tex_upload(TILE1, &mask_surface, &mask_parms);
            rdpq_tex_upload(TILE0, &surf, &tex_params);

            rdpq_texture_rectangle(
                TILE0, 
                x + map_x, y + map_y, 
                x + map_x + MAP_TILE_SIZE, y + map_y + MAP_TILE_SIZE, 
                0, 0
            );
        }
    }
    
    
    material_apply(assets.map_view);

    vector2_t screen_pos;
    map_get_position(player_get_position(&current_scene->player), &screen_pos);

    vector3_t forward;
    quatMultVector(&current_scene->camera.transform.rotation, &gForward, &forward);
    vector2_t cam_rot;
    vector2LookDir(&cam_rot, &forward);
    vector2Negate(&cam_rot, &cam_rot);
    struct view_vertex cursor_points[3];
    for (int i = 0; i < 3; i += 1) {
        menu_transform_point(&player_cursor_points[i], &cam_rot, &screen_pos, &cursor_points[i].pos);
        cursor_points[i].pos.x += map_x;
        cursor_points[i].pos.y += map_y;
        cursor_points[i].col = (vector4_t){
            0.5f, 0.5f, 0.0f,
            i == 0 ? 0.75f : 0.0f
        };
    }
    rdpq_triangle(
        &TRIFMT_SHADE, 
        (float*)&cursor_points[0], 
        (float*)&cursor_points[1], 
        (float*)&cursor_points[2]
    );
    
    material_apply(assets.map_arrow);

    map_get_position(player_get_position(&current_scene->player), &screen_pos);

    vector2_t* rot = player_get_rotation(&current_scene->player);
    for (int i = 0; i < 3; i += 1) {
        menu_transform_point(&player_cursor_points[i], rot, &screen_pos, &cursor_points[i].pos);
        cursor_points[i].pos.x += map_x;
        cursor_points[i].pos.y += map_y;
    }
    rdpq_triangle(
        &TRIFMT_FILL, 
        (float*)&cursor_points[0], 
        (float*)&cursor_points[1], 
        (float*)&cursor_points[2]
    );
}

void map_render_title(struct menu_item* item) {
    if (!item) {
        return;
    }

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_RIGHT,
            .valign = VALIGN_BOTTOM,
            .width = 128,
            .height = 0,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        MENU_X, MAP_Y - 4, 
        item->name,
        strlen(item->name)
    );
}

void map_render_details(struct menu_item* item) {
    switch (item->type) {
        case MENU_ITEM_PART:
            rdpq_text_printn(&(rdpq_textparms_t){
                    // .line_spacing = -3,
                    .align = ALIGN_LEFT,
                    .valign = VALIGN_TOP,
                    .width = 128,
                    .height = 128,
                    .wrap = WRAP_NONE,
                }, 
                FONT_DIALOG, 
                MENU_X, MAP_Y, 
                item->data.part.description,
                strlen(item->data.part.description)
            );
            break;
        case MENU_ITEM_MAP:
            if (map_menu.details_image) {
                material_apply(assets.map_icon);

                int x = (MAP_SIZE - map_menu.details_image->width) >> 1;
                int y = (MAP_SIZE - map_menu.details_image->height) >> 1;
                rdpq_sprite_blit(map_menu.details_image, MENU_X + x, MAP_Y + y, NULL);
            }
            break;
        default:
            break;
    }
}

bool map_should_show_item(struct menu_item* item) {
    return inventory_has_item(item->inventory_item) && !inventory_has_item(item->hide_override);
}

#define FADE_IN_RATIO   0.3f

void map_render_items(float lerp_amount) {
    int x = 0;
    int y = 0;
    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        struct menu_item* item = &menu_items[i];
        
        if (!map_should_show_item(&menu_items[i])) {
            continue;
        }

        if (item->inventory_item == map_menu.selected_item) {
            material_apply(assets.selection_cursor);
            rdpq_texture_rectangle(TILE0, x + MENU_X, y + MAP_Y, x + MENU_X + ICON_SIZE, y + MAP_Y + ICON_SIZE, 0, 0);
        }

        material_apply(assets.map_icon);

        if (!map_menu.has_prev[i]) {
            color_t prim_color;
            if (lerp_amount >= 1.0f) {
                prim_color = (color_t){0, 0, 0, 255};
            } else if (lerp_amount < FADE_IN_RATIO) {
                prim_color = coloru8_lerp(&(color_t){
                    255, 255, 255, 0,
                }, &(color_t){
                    255, 255, 255, 255,
                }, lerp_amount * (1.0f / FADE_IN_RATIO));
            } else {
                prim_color = coloru8_lerp(&(color_t){
                    255, 255, 255, 255,
                }, &(color_t){
                    0, 0, 0, 255,
                }, (lerp_amount - FADE_IN_RATIO) * (1.0f / (1.0f - FADE_IN_RATIO)));
            }

            rdpq_set_prim_color(prim_color);
        }

        rdpq_sprite_blit(assets.icons[item->type], x + MENU_X, y + MAP_Y, NULL);

        x += ICON_SIZE;

        if (x >= MAP_SIZE) {
            x = 0;
            y += ICON_SIZE;
        }
    }
}

enum inventory_item_type map_get_default_selection() {
    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        if (!map_should_show_item(&menu_items[i])) {
            continue;
        }
        return menu_items[i].inventory_item;
    }

    return ITEM_TYPE_NONE;
}

struct menu_item* map_find_selected_item() {
    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        if (!map_should_show_item(&menu_items[i])) {
            continue;
        }
        if (menu_items[i].inventory_item == map_menu.selected_item) {
            return &menu_items[i];
        }
    }

    return NULL;
}

void map_render(void* data) {
    menu_common_render_background(26, 26, 268, 188);

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_RIGHT,
            .valign = VALIGN_BOTTOM,
            .width = 128,
            .height = 0,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        MAP_X, MAP_Y - 4, 
        "Map",
        3
    );

    map_render_minimap(MAP_X, MAP_Y);

    struct menu_item* selected_item = map_find_selected_item();
    map_render_title(selected_item);

    switch (map_menu.state) {
        case MAP_MENU_LIST:
            map_render_items(1.0f);
            break;
        case MAP_MENU_NEW_ITEMS:
        case MAP_MENU_NEW_ITEM_DETAILS:
            map_render_items(map_menu.state_data.new_items.timer * (1.0f / NEW_ITEM_ANIM_TIME));
            break;
        case MAP_MENU_DETAILS_ANIMATE:
        case MAP_MENU_DETAILS:
            map_render_details(selected_item);
            break;
    }
}

void map_menu_init() {
    for (int y = 0; y < MAP_SIZE; y += 1) {
        for (int x = 0; x < MAP_SIZE; x += 1) {
            map_revealed[x + y * MAP_SIZE] = 0;
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
                *pixel = 255 - (uint8_t)(255.0f * (distance - (BRUSH_HALF_SIZE - BLUR_RADIUS)) * (1.0f / BLUR_RADIUS));
            }

            pixel += 1;
        }
    }

    map_menu.selected_item = 0;
    
    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        map_menu.has_prev[i] = map_should_show_item(&menu_items[i]);
    }
}

void map_menu_destroy() {

}

void map_menu_show_details() {
    struct menu_item* selected = map_find_selected_item();

    if (!selected) {
        return;
    }

    if (map_menu.state == MAP_MENU_NEW_ITEMS) {
        map_menu.state = MAP_MENU_NEW_ITEM_DETAILS;
    } else {
        map_menu.state = MAP_MENU_DETAILS;
        if (selected->type == MENU_ITEM_MAP) {
            map_menu.details_image = sprite_load(selected->data.map.image_filename);
        }
    }
}

void map_menu_hide_details() {
    if (map_menu.details_image) {
        sprite_free(map_menu.details_image);
        map_menu.details_image = NULL;
    }

    map_menu.state = MAP_MENU_LIST;
}

void map_menu_animate_new_items(bool show_details) {
    map_menu.state = show_details ? MAP_MENU_NEW_ITEM_DETAILS : MAP_MENU_NEW_ITEMS;
    map_menu.state_data = (union map_menu_state_data){
        .new_items = {
            .timer = 0.0f,
        },
    };
}

void map_menu_update(void* data) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);
    joypad_inputs_t input = joypad_get_inputs(0);
    
    if (!input.btn.start) {
        map_menu.can_unpause = true;
    }

    switch (map_menu.state) {
        case MAP_MENU_LIST:
            if ((pressed.start && map_menu.can_unpause) || pressed.b) {
                map_menu_hide();
                return;
            }
            if (pressed.a) {
                map_menu_show_details();
            }
            break;
        case MAP_MENU_NEW_ITEMS:
            map_menu.state_data.new_items.timer += fixed_time_step;
            if (map_menu.state_data.new_items.timer >= NEW_ITEM_ANIM_TIME) {
                map_menu.state = MAP_MENU_LIST;
            }
            break;
        case MAP_MENU_NEW_ITEM_DETAILS:
            map_menu.state_data.new_items.timer += fixed_time_step;
            if (map_menu.state_data.new_items.timer >= NEW_ITEM_ANIM_TIME + OPEN_ITEM_DELAY_TIME) {
                map_menu.state = MAP_MENU_LIST;
                map_menu_show_details();
            }
            break;
        case MAP_MENU_DETAILS_ANIMATE:
            map_menu.state = MAP_MENU_DETAILS;
            break;
        case MAP_MENU_DETAILS:
            if (pressed.start && map_menu.can_unpause) {
                map_menu_hide();
                return;
            }
            if (pressed.b) {
                map_menu_hide_details();
            }
            break;
    }
}

void map_menu_show_with_item(enum inventory_item_type item) {
    if (current_scene) {
        current_scene->can_pause = false;
    }

    assets.map = sprite_load("rom:/images/menu/map.sprite");
    assets.material = material_cache_load("rom:/materials/menu/map.mat");
    assets.map_background = material_cache_load("rom:/materials/menu/map_grid.mat");
    assets.map_arrow = material_cache_load("rom:/materials/menu/map_arrow.mat");
    assets.map_view = material_cache_load("rom:/materials/menu/map_view.mat");
    assets.map_icon = material_cache_load("rom:/materials/menu/map_icon.mat");
    assets.selection_cursor = material_cache_load("rom:/materials/menu/selection_cursor.mat");
    
    map_menu.details_image = NULL;
    map_menu.state = MAP_MENU_LIST;

    bool has_new = false;
    bool should_show_details = false;

    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        bool should_show = map_should_show_item(&menu_items[i]);

        if (should_show && !map_menu.has_prev[i]) {
            if (menu_items[i].inventory_item == item && menu_items[i].type == MENU_ITEM_MAP) {
                should_show_details = true;
            }            

            has_new = true;
            break;
        }
    }

    if (has_new) {
        map_menu_animate_new_items(should_show_details);
    }

    for (int i = 0; i < MENU_ITEM_TYPE_COUNT; i += 1) {
        assets.icons[i] = sprite_load(icon_files[i]);
    }

    update_pause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    update_unpause_layers(UPDATE_LAYER_PAUSE_MENU);
    game_mode_enter_menu();
    menu_add_callback(map_render, &map_menu, MENU_PRIORITY_OVERLAY);
    update_add(&map_menu, map_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_PAUSE_MENU);
    font_type_use(FONT_DIALOG);
    map_menu.can_unpause = false;

    if (item != ITEM_TYPE_NONE) {
        map_menu.selected_item = item;
    } else if (map_menu.selected_item == ITEM_TYPE_NONE) {
        map_menu.selected_item = map_get_default_selection();
    }
}

void map_menu_show() {
    map_menu_show_with_item(ITEM_TYPE_NONE);
}

void map_menu_hide() {
    sprite_free(assets.map);
    material_cache_release(assets.material);
    material_cache_release(assets.map_background);
    material_cache_release(assets.map_arrow);
    material_cache_release(assets.map_view);
    material_cache_release(assets.map_icon);
    material_cache_release(assets.selection_cursor);
    assets.map = NULL;
    assets.material = NULL;
    assets.map_background = NULL;
    assets.map_arrow = NULL;
    assets.map_view = NULL;
    assets.map_icon = NULL;
    assets.selection_cursor = NULL;

    if (map_menu.details_image) {
        sprite_free(map_menu.details_image);
        map_menu.details_image = NULL;
    }

    for (int i = 0; i < MENU_ITEM_TYPE_COUNT; i += 1) {
        sprite_free(assets.icons[i]);
        assets.icons[i] = NULL;
    }
    
    for (int i = 0; i < MENU_ITEM_COUNT; i += 1) {
        map_menu.has_prev[i] = map_should_show_item(&menu_items[i]);
    }

    update_unpause_layers(UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
    update_pause_layers(UPDATE_LAYER_PAUSE_MENU);
    game_mode_exit_menu();
    menu_remove_callback(&map_menu);
    update_remove(&map_menu);
    font_type_release(FONT_DIALOG);
}

void map_mark_revealed(struct Vector3* pos) {
    if (!current_scene || !current_scene->overworld) {
        return;
    }

    vector2_t screen_pos;
    map_get_position(pos, &screen_pos);

    int center_x = (int)screen_pos.x;
    int center_y = (int)screen_pos.y;

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