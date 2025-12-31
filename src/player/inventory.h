#ifndef __PLAYER_INVENTORY_H__
#define __PLAYER_INVENTORY_H__

#include <stdint.h>
#include "../scene/scene_definition.h"
#include "../cutscene/evaluation_context.h"

struct global_location { 
    uint16_t data_type;
    uint16_t word_offset;
};

bool inventory_has_item(enum inventory_item_type item);
void inventory_give_item(enum inventory_item_type item);

#endif