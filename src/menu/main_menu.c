#include "main_menu.h"

#include <libdragon.h>
#include "../render/material.h"
#include "../resource/material_cache.h"
#include "menu_rendering.h"
#include "menu_common.h"
#include "../time/time.h"
#include "../fonts/fonts.h"
#include "../scene/scene.h"
#include "../render/defs.h"
#include "../effects/fade_effect.h"

struct main_menu {
    bool is_showing;
    bool has_game;
    sprite_t* title;
    sprite_t* title_band;
    material_t* title_material;
    
    int selected_item;
    float go_timer;
    float intro_timer;
};

static struct main_menu main_menu;

#define TITLE_Y         24
#define BAND_OFFSET     24
#define TITLE_X         40

#define TEXT_X              120
#define TEXT_LINE_0_Y       130
#define TEXT_LINE_SPACING   20

#define SLIDE_IN_TIME       0.25f
#define FLASH_TIME          0.5f

#define INTRO_TIME          (SLIDE_IN_TIME + FLASH_TIME)

void main_menu_render(void* data) {
    material_apply(main_menu.title_material);

    int flash_alpha = 0;

    if (main_menu.intro_timer > FLASH_TIME) {
        rdpq_set_prim_color((color_t){255, 255, 255, 255});
    } else if (main_menu.intro_timer > 0.0f) {
        flash_alpha = ((int)((255.0f / FLASH_TIME) * main_menu.intro_timer));
        rdpq_set_prim_color((color_t){flash_alpha, flash_alpha, flash_alpha, 255});
    }

    float slide_amount = 0.0f;

    if (main_menu.intro_timer > FLASH_TIME) {
        slide_amount = (main_menu.intro_timer - FLASH_TIME) * (1.0f / SLIDE_IN_TIME);
    }

    rdpq_blitparms_t band_params = (rdpq_blitparms_t){
        .scale_y = main_menu.intro_timer > FLASH_TIME ? (1.0f - slide_amount) : 1.0f,
    };
    if (slide_amount < 0.99f) {
        rdpq_sprite_blit(main_menu.title_band, 0, TITLE_Y + BAND_OFFSET, &band_params);
    }
    rdpq_sprite_blit(main_menu.title, TITLE_X + slide_amount * SCREEN_WD, TITLE_Y, NULL);

    if (main_menu.intro_timer > FLASH_TIME) {
        return;
    }

    if (flash_alpha) {
        material_apply(solid_primitive_material);
        rdpq_set_prim_color((color_t){255, 255, 255, flash_alpha});
        
        rdpq_texture_rectangle(TILE0, 0, 0, SCREEN_WD, SCREEN_HT, 0, 0);
    }

    menu_common_render_background(100, 120, 120, 60);

    if (main_menu.has_game) {
        rdpq_text_printn(&(rdpq_textparms_t){
                .align = ALIGN_LEFT,
                .valign = VALIGN_TOP,
                .width = 128,
                .height = 40,
                .wrap = WRAP_NONE,
            }, 
            FONT_DIALOG, 
            TEXT_X, TEXT_LINE_0_Y, 
            "Continue",
            strlen("Continue")
        );
    }
    
    rdpq_text_printn(&(rdpq_textparms_t){
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 128,
            .height = 40,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        TEXT_X, TEXT_LINE_0_Y + (main_menu.has_game ? TEXT_LINE_SPACING : 0), 
        "New game",
        strlen("New game")
    );


    material_apply(menu_icons_material);

    rdpq_texture_rectangle(
        TILE0,
        TEXT_X - 16, TEXT_LINE_0_Y + main_menu.selected_item * TEXT_LINE_SPACING,
        TEXT_X, TEXT_LINE_0_Y + main_menu.selected_item * TEXT_LINE_SPACING + 16,
        32, 0
    );
}

void main_menu_update(void *data) {
    static int prev_y;

    joypad_inputs_t inputs = joypad_get_inputs(0);

    if (main_menu.has_game) {
        if ((inputs.stick_y > 40 && prev_y <= 40) || (inputs.stick_y < -40 && prev_y >= -40)) {
            main_menu.selected_item = 1 - main_menu.selected_item;
        }
    }

    prev_y = inputs.stick_y;

    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.a) {
        fade_effect_set((color_t){0, 0, 0, 255}, 1.0f);
        main_menu.go_timer = 1.0f;
    }

    if (main_menu.go_timer > 0.0f) {
        main_menu.go_timer -= fixed_time_step;

        if (main_menu.go_timer < 0.0f) {
            main_menu_hide();
            scene_queue_next("rom:/scenes/intro.scene#default");
        }
    }

    if (main_menu.intro_timer > 0.0f) {
        main_menu.intro_timer -= fixed_time_step;
    }
}

void main_menu_show() {
    if (main_menu.is_showing) {
        return;
    }
    main_menu.is_showing = true;
    main_menu.has_game = false;
    main_menu.selected_item = 0;
    main_menu.go_timer = 0.0f;
    main_menu.intro_timer = INTRO_TIME;

    menu_add_callback(main_menu_render, &main_menu, MENU_PRIORITY_OVERLAY);
    main_menu.title = sprite_load("rom:/images/menu/game_title.sprite");
    main_menu.title_band = sprite_load("rom:/images/menu/game_title_band.sprite");
    main_menu.title_material = material_cache_load("rom:/materials/menu/map_icon.mat");

    font_type_use(FONT_DIALOG);
    update_add(&main_menu, main_menu_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_CUTSCENE | UPDATE_LAYER_PAUSE_MENU);
}

void main_menu_hide() {
    if (!main_menu.is_showing) {
        return;
    }
    main_menu.is_showing = false;
    menu_remove_callback(&main_menu);

    sprite_free(main_menu.title);
    sprite_free(main_menu.title_band);
    material_cache_release(main_menu.title_material);
    update_remove(&main_menu);

    font_type_release(FONT_DIALOG);
}
