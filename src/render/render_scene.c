#include "render_scene.h"

#include "../util/blist.h"
#include <malloc.h>
#include <stdbool.h>
#include "defs.h"

#define MIN_RENDER_SCENE_SIZE   64

struct render_scene r_scene_3d;

void render_scene_reset() {
    callback_list_reset(&r_scene_3d.callbacks, sizeof(struct render_scene_element), MIN_RENDER_SCENE_SIZE, NULL);
    callback_list_reset(&r_scene_3d.step_callbacks, sizeof(struct render_scene_step), MIN_RENDER_SCENE_SIZE, NULL);
}

void render_scene_add(struct Vector3* center, float radius, render_scene_callback callback, void* data) {
    struct render_scene_element element;

    element.data = data;
    element.center = center;
    element.radius = radius;

    callback_list_insert_with_id(&r_scene_3d.callbacks, callback, &element, (callback_id)data);
}

void render_scene_render_renderable(void* data, struct render_batch* batch) {
    struct renderable* renderable = (struct renderable*)data;

    if (renderable->hide) {
        return;
    }
    
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformToWorldMatrix(renderable->transform.transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    struct render_batch_element* element = render_batch_add_tmesh(
        batch, 
        renderable->mesh, 
        mtxfp, 
        &renderable->armature, 
        renderable->attachments,
        renderable->attrs
    );

    if (element && renderable->force_material) {
        element->material = renderable->force_material;
    }
}

void render_scene_render_renderable_single_axis(void* data, struct render_batch* batch) {
    struct renderable* renderable = (struct renderable*)data;

    if (renderable->hide) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    mat4x4 mtx;
    transformSAToMatrix(renderable->transform.transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    struct render_batch_element* element = render_batch_add_tmesh(
        batch, 
        renderable->mesh, 
        mtxfp, 
        &renderable->armature, 
        renderable->attachments,
        renderable->attrs
    );

    if (element && renderable->force_material) {
        element->material = renderable->force_material;
    }
}

// removed with render_scene_remove()
void render_scene_add_renderable(struct renderable* renderable, float radius) {
    // remove with render_scene_remove()
    render_scene_add(
        transform_mixed_get_position(&renderable->transform), 
        radius, 
        renderable->type == TRANSFORM_TYPE_BASIC ? render_scene_render_renderable : render_scene_render_renderable_single_axis, 
        renderable
    );
}

void render_scene_remove(void* data) {
    callback_list_remove(&r_scene_3d.callbacks, (callback_id)data);
}

void render_scene_add_step(render_step_callback callback, void* data) {
    struct render_scene_step step = {data};
    callback_list_insert_with_id(&r_scene_3d.step_callbacks, callback, &step, (callback_id)data);
}

void render_scene_remove_step(void* data) {
    callback_list_remove(&r_scene_3d.step_callbacks, (callback_id)data);
}

#define FX_SCALE    32

void render_scene_render(struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    struct render_batch batch;

    struct ClippingPlanes clipping_planes;
    mat4x4 view_proj_matrix;

    camera_apply(camera, viewport, &clipping_planes, view_proj_matrix);

    t3d_viewport_attach(viewport);
    // just in case I need this fix
    t3d_state_set_vertex_fx_scale(FX_SCALE);

    struct callback_element* current_step = callback_list_get(&r_scene_3d.step_callbacks, 0);

    for (int i = 0; i < r_scene_3d.step_callbacks.count; ++i) {
        struct render_scene_step* step = callback_element_get_data(current_step);
        ((render_step_callback)current_step->callback)(step->data, view_proj_matrix, camera, viewport, pool);

        current_step = callback_list_next(&r_scene_3d.step_callbacks, current_step);
    }

    render_batch_init(&batch, &camera->transform, pool);

    struct callback_element* current = callback_list_get(&r_scene_3d.callbacks, 0);

    for (int i = 0; i < r_scene_3d.callbacks.count; ++i) {
        struct render_scene_element* el = callback_element_get_data(current);
        ((render_scene_callback)current->callback)(el->data, &batch);

        current = callback_list_next(&r_scene_3d.callbacks, current);
    }
    render_batch_finish(&batch, view_proj_matrix, viewport);
}