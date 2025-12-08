#include "debug_colliders.h"

#include "../render/tmesh.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../resource/tmesh_cache.h"
#include "../scene/scene.h"
#include "../render/defs.h"

static struct tmesh* bb_mesh;

void debug_render_colliders(void* data, struct render_batch* batch) {
    for (int i = 0; i < collision_scene_get_count(); i += 1) {
        struct collision_scene_element* element = collision_scene_get_element(i);

        struct Box3D* bb = NULL;

        if (!element || !element->object) {
            continue;
        }

        if (element->type == COLLISION_ELEMENT_TYPE_DYNAMIC) {
            bb = &(((struct dynamic_object*)element->object)->bounding_box);
        } else if (element->type == COLLISION_ELEMENT_TYPE_TRIGGER) {
            bb = &(((struct spatial_trigger*)element->object)->bounding_box);
        }

        if (!bb) {
            continue;;
        }

        T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

        if (!mtxfp) {
            return;
        }

        mat4x4 mtx;

        matrixFromScale(mtx, MODEL_WORLD_SCALE);
        mtx[0][0] = (bb->max.x - bb->min.x) * MODEL_WORLD_SCALE;
        mtx[1][1] = (bb->max.y - bb->min.y) * MODEL_WORLD_SCALE;
        mtx[2][2] = (bb->max.z - bb->min.z) * MODEL_WORLD_SCALE;
        matrixApplyScaledPos(mtx, &bb->min, WORLD_SCALE);
        render_batch_relative_mtx(batch, mtx);
        t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);
        
        render_batch_add_tmesh(batch, bb_mesh, mtxfp, NULL, NULL, NULL);
    }
}

void debug_collider_enable() {
    if (bb_mesh) {
        return;
    }
    bb_mesh = tmesh_cache_load("rom:/meshes/test/bounding_box.tmesh");
    render_scene_add(NULL, 0.0f, debug_render_colliders, bb_mesh);
}

void debug_collider_disable() {
    if (bb_mesh) {
        return;
    }

    render_scene_remove(bb_mesh);
    tmesh_cache_release(bb_mesh);
    bb_mesh = NULL;
}