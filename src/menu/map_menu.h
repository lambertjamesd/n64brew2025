#ifndef __MENU_MAP_MENU_H__
#define __MENU_MAP_MENU_H__

#include "../math/vector3.h"
#include "../scene/scene_definition.h"

void map_menu_init();
void map_menu_destroy();

void map_menu_show();
void map_menu_show_with_item(enum inventory_item_type item);
void map_menu_hide();

void map_mark_revealed(struct Vector3* pos);

#endif