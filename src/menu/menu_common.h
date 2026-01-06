#ifndef __MENU_MENU_COMMON_H__
#define __MENU_MENU_COMMON_H__

#include "../render/material.h"
#include "../math/vector2.h"
#include "../math/vector4.h"

extern struct material* menu_icons_material;
extern struct material* solid_primitive_material;
extern struct material* sprite_blit;

struct view_vertex {
    vector2_t pos;
    vector4_t col;
};

void menu_common_init();

void menu_common_render_background(int x, int y, int w, int h);

void menu_transform_point(vector2_t* input, vector2_t* rotation, vector2_t* screen_pos, vector2_t* output);

#endif