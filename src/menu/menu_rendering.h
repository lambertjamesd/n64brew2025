#ifndef __MENU_MENU_RENDERING_H__
#define __MENU_MENU_RENDERING_H__

typedef void (*menu_render_callback)(void* data);

enum menu_priority {
    MENU_PRIORITY_HUD,
    MENU_PRIORITY_OVERLAY,
    MENU_PRIORITY_DIALOG,
    MENU_PRIORITY_TITLE,
};

void menu_reset();

void menu_add_callback(menu_render_callback callback, void* data, enum menu_priority priority);
void menu_remove_callback(void* data);

void menu_render();

#endif