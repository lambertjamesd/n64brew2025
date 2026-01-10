#include "motorcycle_spawner.h"
#include "../entity/entity_spawner.h"

#include "../scene/scene.h"
#include "../player/inventory.h"
    
void motorcycle_spawner_init(motorcycle_spawner_t* motorcycle_spawner, struct motorcycle_spawner_definition* definition, entity_id entity_id) {
    entity_spawn_singleton(ENTITY_TYPE_motorcycle, definition, ENTITY_ID_MOTORCYLE);
    motorcycle_spawner->position = definition->position;
}

void motorcycle_spawner_destroy(motorcycle_spawner_t* motorcycle_spawner, struct motorcycle_spawner_definition* definition) {
    
}

void motorcycle_spawner_common_init() {

}

void motorcycle_spawner_common_destroy() {

}
