#include "sign.h"

#include "../render/render_scene.h"
#include "../cutscene/cutscene_runner.h"
#include "../collision/shapes/box.h"
#include "../collision/collision_scene.h"

static struct dynamic_object_type sign_collider = {
    BOX_COLLIDER(0.1625f, 0.325f, 0.05f),
};

void sign_interact(struct interactable* interactable, entity_id from) {
    sign_t* sign = (sign_t*)interactable->data;
    cutscene_ref_run(&sign->cutscene, sign->entity_id);
}

void sign_init(sign_t* sign, struct sign_definition* def, entity_id entity_id) {
    transformSaInit(&sign->transform, &def->position, &def->rotation, 1.0f);
    sign->entity_id = entity_id;

    interactable_init(&sign->interactable, entity_id, def->message ? INTERACT_TYPE_READ : INTERACT_TYPE_NONE, sign_interact, sign);

    renderable_single_axis_init(&sign->renderable, &sign->transform, "rom:/meshes/objects/text_poster.tmesh");
    render_scene_add_renderable(&sign->renderable, 1.0f);

    dynamic_object_init(
        entity_id,
        &sign->dynamic_object,
        &sign_collider,
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_Z_TARGET,
        &sign->transform.position,
        &sign->transform.rotation
    );

    sign->dynamic_object.is_fixed = 1;
    sign->dynamic_object.weight_class = WEIGHT_CLASS_SUPER_HEAVY;

    collision_scene_add(&sign->dynamic_object);

    cutscene_ref_init(&sign->cutscene, def->message);
}

void sign_destroy(sign_t* sign) {
    interactable_destroy(&sign->interactable);
    renderable_destroy(&sign->renderable);
    render_scene_remove(&sign->renderable);
    collision_scene_remove(&sign->dynamic_object);
    cutscene_ref_destroy(&sign->cutscene);
}

void sign_common_init() {

}

void sign_common_destroy() {

}