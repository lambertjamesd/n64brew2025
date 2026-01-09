#ifndef __ENTITIES_MOTORCYCLE_SPAWNER_H__
#define __ENTITIES_MOTORCYCLE_SPAWNER_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

struct motorcycle_spawner {
    vector3_t position;
};

typedef struct motorcycle_spawner motorcycle_spawner_t;

void motorcycle_spawner_init(motorcycle_spawner_t* motorcycle_spawner, struct motorcycle_spawner_definition* definition, entity_id entity_id);
void motorcycle_spawner_destroy(motorcycle_spawner_t* motorcycle_spawner, struct motorcycle_spawner_definition* definition);
void motorcycle_spawner_common_init();
void motorcycle_spawner_common_destroy();

#endif