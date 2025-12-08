#ifndef __OBJECTS_EMPTY_H__
#define __OBJECTS_EMPTY_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

struct empty {
    struct Vector3 position;
    struct Vector2 rotation;
    entity_id entity_id;
};

typedef struct empty empty_t;

void empty_init(struct empty* empty, struct empty_definition* definition, entity_id entity_id);
void empty_destroy(struct empty* empty, struct empty_definition* definition);
void empty_common_init();
void empty_common_destroy();

empty_t* empty_find(entity_id entity_id);

#endif