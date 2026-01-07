#include "cutscene_timer.h"

#include <string.h>
#include "../time/time.h"
#include "cutscene_reference.h"
#include "../menu/menu_rendering.h"
#include "../fonts/fonts.h"

#define MAX_CUTSCENE_NAME_LEN   64

struct cutscene_timer {
    bool is_active;
    char on_end[MAX_CUTSCENE_NAME_LEN];
    float time_left;
};

static struct cutscene_timer timer;

void cutscene_timer_render(void* data) {
    int minutes = 0;
    int seconds = 0;

    char time[20];

    seconds = (int)ceilf(timer.time_left <= 0.0f ? 0.0f : timer.time_left);
    minutes = seconds / 60;
    seconds = seconds % 60;

    sprintf(time, "%02d:%02d", minutes, seconds);
    
    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 260,
            .height = 60,
            .wrap = WRAP_NONE,
        }, 
        FONT_DIALOG, 
        30, 30, 
        time,
        strlen(time)
    );
}

void cutscene_timer_update(void* data) {
    float before = timer.time_left;

    timer.time_left -= scaled_time_step;

    if (before > 0.0f && timer.time_left <= 0.0f) {
        if (timer.on_end[0]) {
            cutscene_ref_t cutscene;
            cutscene_ref_init(&cutscene, timer.on_end);
            cutscene_ref_run_then_destroy(&cutscene, 0);
        }

        cutscene_timer_cancel();
    }

    if (timer.time_left < -2.0f) {
        cutscene_timer_cancel();
    }
}

void cutscene_timer_start(float time, const char* on_end) {
    if (cutscene_timer_is_active()) {
        return;
    }

    if (on_end) {
        assert(strlen(on_end) + 1 < MAX_CUTSCENE_NAME_LEN);
        strcpy(timer.on_end, on_end);
    } else {
        timer.on_end[0] = '\0';
    }
    timer.time_left = time;
    timer.is_active = true;
    update_add(&timer, cutscene_timer_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
    menu_add_callback(cutscene_timer_render, &timer, MENU_PRIORITY_HUD);
    font_type_use(FONT_DIALOG);
}

void cutscene_timer_cancel() {
    if (!cutscene_timer_is_active()) {
        return;
    }

    update_remove(&timer);
    menu_remove_callback(&timer);
    font_type_release(FONT_DIALOG);
    timer.time_left = 0.0f;
    timer.on_end[0] = '\0';
    timer.is_active = false;
}

bool cutscene_timer_is_active() {
    return timer.is_active;
}

float cutscene_timer_remaining() {
    return timer.time_left;
}