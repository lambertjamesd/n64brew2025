#include "dialog_box.h"

#include "../fonts/fonts.h"
#include "menu_common.h"
#include "menu_rendering.h"
#include "../time/time.h"
#include "../util/text.h"
#include <string.h>

#define CHARACTERS_PER_SECOND   45.0f

struct dialog_box dialog_box;

void dialog_box_init() {
    dialog_box.current_message = NULL;
    font_type_use(FONT_DIALOG);
}

void dialog_box_update(void* data) {
    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (!dialog_box.paused) {
        dialog_box.requested_characters += fixed_time_step * (input.btn.a ? CHARACTERS_PER_SECOND * 4.0f : CHARACTERS_PER_SECOND);

        while (dialog_box.requested_characters >= 1.0f) {
            uint32_t character_point = utf8_decode(&dialog_box.current_message_end);

            if (character_point == 0 || (character_point == '\n' && *dialog_box.current_message_end == '\n')) {
                dialog_box.paused = 1;

                if (character_point == 0) {
                    dialog_box.end_of_message = 1;
                }

                dialog_box.requested_characters = 0;
                break;
            }

            dialog_box.requested_characters -= 1.0f;
        }
    }

    if (dialog_box.paused && pressed.a) {
        if (dialog_box.end_of_message) {
            if (dialog_box.end_callback) {
                dialog_box.end_callback(dialog_box.end_callback_data);
            }
            dialog_box_hide();
        } else {
            dialog_box.current_message_start = dialog_box.current_message_end + 1;
            dialog_box.current_message_end = dialog_box.current_message_start;
            dialog_box.paused = 0;
        }
    }
}

void dialog_box_render(void* data) {    
    menu_common_render_background(
        20, 167,
        280,
        53
    );

    rdpq_sync_pipe();

    rdpq_text_printn(&(rdpq_textparms_t){
            // .line_spacing = -3,
            .align = ALIGN_LEFT,
            .valign = VALIGN_TOP,
            .width = 260,
            .height = 60,
            .wrap = WRAP_WORD,
        }, 
        FONT_DIALOG, 
        30, 170, 
        dialog_box.current_message_start,
        dialog_box.current_message_end - dialog_box.current_message_start
    );

    if (dialog_box.paused) {
        material_apply(menu_icons_material);
        rdpq_texture_rectangle(
            TILE0,
            274, 196,
            290, 214,
            dialog_box.end_of_message ? 16 : 0, 0
        );
    }
}

void dialog_box_format_string(char* into, char* format, int* args) {
    while (*format) {
        if (*format == '%') {
            format += 1;

            int arg = *args;
            args += 1;

            if (*format == 's') {
                char* as_str = (char*)arg;
                while (*as_str) *into++ = *as_str++;
            } else if (*format == 'd') {
                into += sprintf(into, "%d", arg);
            } else if (*format == '%') {
                *into++ = '%';
            }
        } else {
            *into++ = *format;
        }

        format += 1;
    }

    *into++ = '\0';
}

void dialog_box_show(char* message, int* args, dialog_end_callback end_callback, void* end_callback_data) {
    dialog_box_format_string(dialog_box.current_text, message, args);
    dialog_box.current_message = dialog_box.current_text;

    menu_add_callback(dialog_box_render, &dialog_box, MENU_PRIORITY_DIALOG);
    update_add(&dialog_box, dialog_box_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_DIALOG);

    dialog_box.current_message_start = dialog_box.current_message;
    dialog_box.current_message_end = dialog_box.current_message;

    dialog_box.requested_characters = 0.0f;
    dialog_box.paused = 0;
    dialog_box.end_of_message = 0;

    dialog_box.end_callback = end_callback;
    dialog_box.end_callback_data = end_callback_data;
}

bool dialog_box_is_active() {
    return dialog_box.current_message != NULL;
}

void dialog_box_hide() {
    menu_remove_callback(&dialog_box);
    update_remove(&dialog_box);
    dialog_box.current_message = NULL;
}

void dialog_box_destroy() {
    rdpq_text_unregister_font(1);
    font_type_release(FONT_DIALOG);
}