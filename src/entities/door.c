#include "door.h"

#include "../collision/shapes/box.h"
#include "../render/render_scene.h"
#include "../collision/collision_scene.h"
#include "../resource/animation_cache.h"
#include "../cutscene/expression_evaluate.h"
#include "../time/time.h"

struct door_type_def {
    const char* mesh_name;
    const char* animations_name;
    dynamic_object_type_t collider;
};

static struct door_type_def types[DOOR_TYPE_COUNT] = {
    [DOOR_TYPE_GARAGE] = {
        .mesh_name = "rom:/meshes/doors/garage_door.tmesh",
        .animations_name = "rom:/meshes/doors/garage_door.anim",
        .collider = {
            BOX_COLLIDER(2.0f, 1.5f, 0.1f),
            .center = {0.0f, 1.5f, 0.0f},
        }
    }
};

void door_common_init() {

}

void door_common_destroy() {

}

void door_update(void* data) {
    door_t* door = (door_t*)data;

    armature_t* armature = renderable_get_armature(&door->renderable);

    animator_update(&door->animator, armature, fixed_time_step);

    bool is_open = expression_get_bool(door->is_open);

    if (is_open & !door->was_open) {
        animator_run_clip(&door->animator, door->animations.open, 0.0f, false);
        collision_scene_remove(&door->collider);
    } else if (!is_open && door->was_open) {
        animator_run_clip(&door->animator, door->animations.close, 0.0f, false);
        collision_scene_add(&door->collider);
    }

    door->was_open = is_open;
}

void door_init(door_t* door, struct door_definition* definition, entity_id entity_id) {
    struct door_type_def* type_def = &types[definition->door_type];

    transformSaInit(&door->transform, &definition->position, &definition->rotation, 1.0f);
    renderable_single_axis_init(&door->renderable, &door->transform, type_def->mesh_name);
    render_scene_add_renderable(&door->renderable, 3.0f);

    animator_init(&door->animator, door->renderable.mesh_render.mesh->armature.bone_count);
    door->animation_set = animation_cache_load(type_def->animations_name);
    door->animations.open = animation_set_find_clip(door->animation_set, "open");
    door->animations.close = animation_set_find_clip(door->animation_set, "close");

    dynamic_object_init(
        entity_id,
        &door->collider,
        &type_def->collider,
        COLLISION_LAYER_TANGIBLE,
        &door->transform.position,
        &door->transform.rotation
    );
    collision_scene_add(&door->collider);
    door->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;
    door->collider.is_fixed = true;

    door->was_open = false;
    door->is_open = definition->is_open;

    update_add(door, door_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);
}

void door_destroy(door_t* door) {
    render_scene_remove(&door->renderable);
    renderable_destroy(&door->renderable);
    if (!door->was_open) {
        collision_scene_remove(&door->collider);
    }
    animation_cache_release(door->animation_set);
    animator_destroy(&door->animator);
    update_remove(door);
}