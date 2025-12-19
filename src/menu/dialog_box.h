#ifndef __MENU_DIALOG_BOX_H__
#define __MENU_DIALOG_BOX_H__

#include <libdragon.h>
#include <stdbool.h>

typedef void (*dialog_end_callback)(void* data);

#define MAX_MESSAGE_CHARACTERS  256

struct dialog_box {
    char* current_message;
    char current_text[MAX_MESSAGE_CHARACTERS];
    rdpq_font_t* font;

    char* current_message_start;
    char* current_message_end;
    float requested_characters;

    uint16_t paused: 1;
    uint16_t end_of_message: 1;

    dialog_end_callback end_callback;
    void* end_callback_data;
};

void dialog_box_init();
void dialog_box_show(char* message, int* args, dialog_end_callback end_callback, void* end_callback_data);
bool dialog_box_is_active();
void dialog_box_hide();
void dialog_box_destroy();

void dialog_box_format_string(char* into, char* format, int* args);

#endif