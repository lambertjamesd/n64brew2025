#include "checkpoint.h"    
    
#include "../render/render_scene.h"
#include "../collision/collision_scene.h"

static spatial_trigger_type_t trigger_type = {
    .type = SPATIAL_TRIGGER_BOX,
    .data = {
        .box = {
            .half_size = {5.0f, 3.0f, 0.5f},
        }
    },
    .center = {0.0f, 2.0f, 0.0f},
};

static const char* checkpoint_meshes[CHECKPOINT_TYPE_COUNT] = {
    [CHECKPOINT_FINISH] = "rom:/meshes/objects/race_start+finish.tmesh",
    [CHECKPOINT_MIDDLE] = "rom:/meshes/objects/race_checkpoint.tmesh",
};

void checkpoint_init(checkpoint_t* checkpoint, struct checkpoint_definition* definition, entity_id entity_id) {
    transformSaInit(&checkpoint->transform, &definition->position, &definition->rotation, 1.0f);
    
    spatial_trigger_init(&checkpoint->trigger, &checkpoint->transform, &trigger_type, COLLISION_LAYER_TANGIBLE, entity_id);
    collision_scene_add_trigger(&checkpoint->trigger);

    renderable_single_axis_init(&checkpoint->renderable, &checkpoint->transform, checkpoint_meshes[definition->checkpoint_type]);
    render_scene_add_renderable(&checkpoint->renderable, 7.0f);
}

void checkpoint_destroy(checkpoint_t* checkpoint, struct checkpoint_definition* definition) {
    collision_scene_remove_trigger(&checkpoint->trigger);

    render_scene_remove(&checkpoint->renderable);
    renderable_destroy(&checkpoint->renderable);
}

void checkpoint_common_init() {

}

void checkpoint_common_destroy() {

}
