#ifndef __MENU_MAP_MENU_H__
#define __MENU_MAP_MENU_H__

#include "../math/vector3.h"

void map_menu_init();
void map_menu_destroy();

void map_menu_show();
void map_menu_hide();

void map_mark_revealed(struct Vector3* pos);

#endif