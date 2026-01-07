#include "health_machine.h"    
    
#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../collision/shapes/box.h"
#include "../cutscene/cutscene_reference.h"

static dynamic_object_type_t collider = {
    BOX_COLLIDER(2.0f, 1.0f, 1.0f),
    .center = {0.0f, 1.0f, 0.0f},
};

void health_machine_interact(interactable_t* interactable, entity_id from) {
    health_machine_t* health_machine = (health_machine_t*)interactable->data;

    cutscene_ref_t cutscene;
    cutscene_ref_init(&cutscene, "rom:/scripts/health_machine_get.script");
    cutscene_ref_run_then_destroy(&cutscene, health_machine->entity_id);
}

void health_machine_init(health_machine_t* health_machine, struct health_machine_definition* definition, entity_id entity_id) {
    transformSaInit(&health_machine->transform, &definition->position, &definition->rotation, 1.0f);
    health_machine->entity_id = entity_id;
    renderable_single_axis_init(&health_machine->renderable, &health_machine->transform, "rom:/meshes/objects/health_juice_machine.tmesh");
    render_scene_add_renderable(&health_machine->renderable, 3.0f);
    
    dynamic_object_init(entity_id, &health_machine->collider, &collider, COLLISION_LAYER_TANGIBLE, &health_machine->transform.position, &health_machine->transform.rotation);
    health_machine->collider.is_fixed = true;
    health_machine->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;
    collision_scene_add(&health_machine->collider);

    interactable_init(&health_machine->interactable, entity_id, INTERACT_TYPE_CHECK, health_machine_interact, health_machine);
}

void health_machine_destroy(health_machine_t* health_machine, struct health_machine_definition* definition) {
    render_scene_remove(&health_machine->renderable);
    renderable_destroy(&health_machine->renderable);

    collision_scene_remove(&health_machine->collider);
    interactable_destroy(&health_machine->interactable);
}

void health_machine_common_init() {

}

void health_machine_common_destroy() {

}
