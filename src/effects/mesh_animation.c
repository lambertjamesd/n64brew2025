#include "mesh_animation.h"

#include "effect_allocator.h"
#include "../render/render_scene.h"
#include "../time/time.h"

struct mesh_animation* mesh_animation_new(struct Vector3* position, struct Vector2* rotation, struct tmesh* mesh, struct animation_clip* clip) {
    struct mesh_animation* result = effect_malloc(sizeof(struct mesh_animation));

    transformSaInit(&result->transform, position, rotation, 1.0f);

    result->renderable.mesh = mesh;
    result->renderable.force_material = NULL;
    armature_init(&result->renderable.armature, &result->renderable.mesh->armature);
    result->renderable.attachments = NULL;
    transform_mixed_init_sa(&result->renderable.transform, &result->transform);
    result->renderable.type = TRANSFORM_TYPE_SINGLE_AXIS;

    render_scene_add_renderable(&result->renderable, 1.0f);

    animator_init(&result->animtor, mesh->armature.bone_count);
    animator_run_clip(&result->animtor, clip, 0.0f, false);

    return result;
}

bool mesh_animation_update(struct mesh_animation* mesh_animation) {
    animator_update(&mesh_animation->animtor, &mesh_animation->renderable.armature, fixed_time_step);
    return animator_is_running(&mesh_animation->animtor);
}

void mesh_animation_free(struct mesh_animation* mesh_animation) {
    render_scene_remove(&mesh_animation->renderable);
    animator_destroy(&mesh_animation->animtor);
    effect_free(mesh_animation);
}