#include "repair_part_pickup.h"

#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"
#include "../cutscene/expression_evaluate.h"
#include "../entity/entity_spawner.h"

struct repair_part_type_def {
    const char* mesh_name;
    dynamic_object_type_t collider;
};

typedef struct repair_part_type_def repair_part_type_def_t;

static repair_part_type_def_t types[REPAIR_PART_COUNT] = {
    [REPAIR_PART_MOTOR] = {
        .mesh_name = "rom:/meshes/parts/motor.tmesh",
        .collider = {
            BOX_COLLIDER(0.2f, 0.2f, 0.3f),
            .center = {0.0f, 0.2f, 0.0f},
        }
    },
    [REPAIR_PART_WATER_PUMP_GEAR] = {
        .mesh_name = "rom:/meshes/parts/motor.tmesh",
        .collider = {
            BOX_COLLIDER(0.5f, 0.1f, 0.5f),
            .center = {0.0f, 0.0f, 0.0f},
        }
    },
};

void repair_part_pickup_common_init() {

}

void repair_part_pickup_common_destroy() {

}

void repair_part_interact(struct interactable* interactable, entity_id from) {
    repair_part_pickup_t* part = (repair_part_pickup_t*)interactable->data;
    expression_set_bool(part->has_part, true);
    entity_despawn(interactable->id);
}

void repair_part_pickup_init(repair_part_pickup_t* part, struct repair_part_pickup_definition* definition, entity_id entity_id) {
    if (expression_get_bool(definition->has_part)) {
        part->is_active = false;
        return;
    }
    part->is_active = true;
    part->has_part = definition->has_part;

    repair_part_type_def_t* def = &types[definition->part_type];

    transformSaInit(&part->transform, &definition->position, &definition->rotation, 1.0f);

    renderable_single_axis_init(&part->renderable, &part->transform, def->mesh_name);
    render_scene_add_renderable(&part->renderable, 1.0f);

    dynamic_object_init(entity_id, &part->collider, &def->collider, COLLISION_LAYER_INTERACT_ONLY, &part->transform.position, &part->transform.rotation);
    collision_scene_add(&part->collider);

    interactable_init(&part->interactable, entity_id, INTERACT_TYPE_TAKE, repair_part_interact, part);
}

void repair_part_pickup_destroy(repair_part_pickup_t* part) {
    if (!part->is_active) {
        return;
    }
    render_scene_remove(&part->renderable);
    renderable_destroy(&part->renderable);
    collision_scene_remove(&part->collider);
    interactable_destroy(&part->interactable);
}