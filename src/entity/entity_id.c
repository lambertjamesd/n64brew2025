#include "entity_id.h"

#include "../scene/scene_definition.h"

static entity_id next_id = ENTITY_ID_FIRST_DYNAMIC;

entity_id entity_id_new() {
    return next_id++;
}