#include "trigger_cube.h"

#include "../collision/collision_scene.h"

void trigger_cube_common_init() {}

void trigger_cube_common_destroy() {}

void trigger_cube_init(trigger_cube_t* cube, struct trigger_cube_definition* definition, entity_id entity_id) {
    transformSaInit(&cube->transform, &definition->position, &definition->rotation, 1.0f);
    cube->type = (spatial_trigger_type_t){
        .type = SPATIAL_TRIGGER_BOX,
        .data = {
            .box = {
                .half_size = definition->scale,
            },
        },
    };

    spatial_trigger_init(&cube->trigger, &cube->transform, &cube->type, COLLISION_LAYER_TANGIBLE, entity_id);
    collision_scene_add_trigger(&cube->trigger);
}

void trigger_cube_destroy(trigger_cube_t* cube) {
    collision_scene_remove_trigger(&cube->trigger);
}