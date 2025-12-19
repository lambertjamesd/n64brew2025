#ifndef __MENU_MENU_COMMON_H__
#define __MENU_MENU_COMMON_H__

#include "../render/material.h"

extern struct material* menu_icons_material;
extern struct material* solid_primitive_material;
extern struct material* sprite_blit;

void menu_common_init();

void menu_common_render_background(int x, int y, int w, int h);

#endif