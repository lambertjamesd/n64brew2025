#include "drop_shadow.h"

#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../math/matrix.h"
#include "../render/defs.h"
#include "../collision/collision_scene.h"
#include "../resource/tmesh_cache.h"

#define SHADOW_SCALE    (0.5f * MODEL_WORLD_SCALE)

void drop_shadow_render(void* data, struct render_batch* batch) {
    struct drop_shadow* drop_shadow = (struct drop_shadow*)data;

    struct contact* contact = dynamic_object_get_ground(drop_shadow->target);

    if (!contact) {
        contact = drop_shadow->target->shadow_contact;
    }

    if (!contact || contact->surface_type == SURFACE_TYPE_WATER || contact->normal.y < 0.1f) {
        return;
    }
    
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    struct Vector3 skew_scale;
    vector3Scale(&contact->normal, &skew_scale, SHADOW_SCALE / contact->normal.y);

    struct Vector3 pos = contact->point;
    pos.y += 0.01f;

    mat4x4 mtx;
    matrixFromScale(mtx, SHADOW_SCALE);
    mtx[0][1] = -skew_scale.x;
    mtx[2][1] = -skew_scale.z;
    matrixApplyScaledPos(mtx, &pos, WORLD_SCALE);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    render_batch_add_tmesh(batch, drop_shadow->mesh, mtxfp, NULL, NULL, NULL);
}

void drop_shadow_init(struct drop_shadow* drop_shadow, struct dynamic_object* target, const char* shadow_mesh) {
    drop_shadow->target = target;

    render_scene_add(target->position, 1.0f, drop_shadow_render, drop_shadow);
    drop_shadow->mesh = tmesh_cache_load(shadow_mesh);
}

void drop_shadow_destroy(struct drop_shadow* drop_shadow) {
    render_scene_remove(drop_shadow);
    tmesh_cache_release(drop_shadow->mesh);
}