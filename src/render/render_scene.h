#ifndef __RENDER_RENDER_SCENE_H__
#define __RENDER_RENDER_SCENE_H__

#include "../math/vector3.h"
#include "renderable.h"
#include "render_batch.h"
#include "camera.h"
#include "../util/callback_list.h"
#include "frame_alloc.h"
#include <t3d/t3d.h>

typedef void (*render_scene_callback)(void* data, struct render_batch* batch);
typedef void (*render_step_callback)(void* data, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool);

typedef int render_id;

struct render_scene_element {
    void* data;
    struct Vector3* center;
    float radius;
};

struct render_scene_step {
    void* data;
};

struct render_scene {
    struct callback_list callbacks;
    struct callback_list step_callbacks;
};

void render_scene_reset();

void render_scene_render_renderable_single_axis(void* data, struct render_batch* batch);

void render_scene_add(struct Vector3* center, float radius, render_scene_callback callback, void* data);
void render_scene_add_renderable(struct renderable* renderable, float radius);
void render_scene_remove(void* data);

void render_scene_add_step(render_step_callback callback, void* data);
void render_scene_remove_step(void* data);

void render_scene_render(struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool);

#endif