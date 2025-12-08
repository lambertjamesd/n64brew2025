#include "area_title.h"

#include <libdragon.h>
#include "../resource/font_cache.h"
#include "../time/time.h"
#include "../menu/menu_rendering.h"

#define MAX_TITLE_LEN   16
#define TITLE_SHOW_TIME 6.0f
#define TITLE_FADE_TIME 1.0f

struct area_title {
    rdpq_font_t* font;
    char message[16];
    float time_left;
};

static struct area_title g_title;

int area_title_measure() {
    const char* curr = g_title.message;

    int result = 0;

    while (*curr) {
        rdpq_font_gmetrics_t metrics;
        bool was_found = rdpq_font_get_glyph_metrics(g_title.font, *curr, &metrics);
        ++curr;

        if (was_found) {
            result += metrics.xadvance;
        }
    }
    
    return result;
}

void area_title_render(void* data) {
    float alpha = 1.0f;

    if (g_title.time_left < TITLE_FADE_TIME) {
        alpha = g_title.time_left * (1.0f / TITLE_FADE_TIME);
    }

    if (g_title.time_left > TITLE_SHOW_TIME - TITLE_FADE_TIME) {
        alpha = (TITLE_SHOW_TIME - g_title.time_left) * (1.0f / TITLE_FADE_TIME);
    }

    rdpq_font_style(g_title.font, 0, &(rdpq_fontstyle_t) {
        .color = {255, 255, 255, (uint8_t)(alpha * 255.0f)},
        .custom = NULL,
        .custom_arg = NULL,
        .outline_color = {255, 255, 255, 255},
    });
    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_CENTER,
            .valign = VALIGN_TOP,
            .width = 320,
            .height = 60,
            .wrap = WRAP_WORD,
        }, 
        2, 
        0, 40, 
        g_title.message,
        strlen(g_title.message)
    );
    
    int half_width = (int)((area_title_measure() >> 1) * alpha);

    rdpq_sync_pipe();
    rdpq_mode_combiner(RDPQ_COMBINER1((0,0,0,PRIM), (0,0,0,PRIM)));

    rdpq_texture_rectangle(TILE0, 160 - half_width, 36, 160 + half_width, 37, 0, 0);
    rdpq_texture_rectangle(TILE0, 160 - half_width, 72, 160 + half_width, 73, 0, 0);
}

void area_title_update(void* data) {
    g_title.time_left -= fixed_time_step;

    if (g_title.time_left <= 0.0f) {
        area_title_hide();
    }
}

void area_title_show(const char* title) {
    strncpy(g_title.message, title, MAX_TITLE_LEN);
    g_title.message[MAX_TITLE_LEN - 1] = '\0';
    g_title.time_left = TITLE_SHOW_TIME;

    if (g_title.font) {
        return;
    }

    g_title.font = font_cache_load("rom:/fonts/HeavyEquipment.font64");
    rdpq_text_register_font(2, g_title.font);
    menu_add_callback(area_title_render, &g_title, MENU_PRIORITY_TITLE);
    update_add(&g_title, area_title_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_CUTSCENE);
}

void area_title_hide() {
    g_title.time_left = 0;

    if (g_title.font) {
        rspq_call_deferred((void (*)(void*))font_cache_release, g_title.font);
        g_title.font = NULL;
        menu_remove_callback(&g_title);
        update_remove(&g_title);
    }
}