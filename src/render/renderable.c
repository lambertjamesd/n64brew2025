#include "renderable.h"

#include "../resource/tmesh_cache.h"
#include "../resource/material_cache.h"
#include "../render/defs.h"
#include <stddef.h>

void _renderable_init(struct renderable* renderable) {
    renderable->mesh_render.force_material = NULL;
    renderable->attrs = NULL;
    armature_init(&renderable->mesh_render.armature, &renderable->mesh_render.mesh->armature);

    if (renderable->mesh_render.mesh->attatchment_count) {
        renderable->mesh_render.attachments = malloc(sizeof(struct tmesh*) * renderable->mesh_render.mesh->attatchment_count);
        memset(renderable->mesh_render.attachments, 0, sizeof(struct tmesh*) * renderable->mesh_render.mesh->attatchment_count);
    } else {
        renderable->mesh_render.attachments = NULL;
    }
    renderable->hide = 0;
}

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    transform_mixed_init(&renderable->transform, transform);
    renderable->mesh_render.mesh = tmesh_cache_load(mesh_filename);
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_BASIC;
}

// released with renderable_destroy_point()
void renderable_init_point(struct renderable* renderable, vector3_t* position, float radius, const char* material_filename) {
    transform_mixed_init_pos(&renderable->transform, position);
    renderable->type = TRANSFORM_TYPE_POSITION;
    renderable->attrs = NULL;
    renderable->hide = 0;
    renderable->point_render.frame_max_x = 0;
    renderable->point_render.frame_step = 0;

    renderable->point_render.particle_data = (TPXParticleS8){
        .sizeA = 127,
        .sizeB = 127,
        .colorA = {255, 255, 255, 0},
        .colorB = {255, 255, 255, 0},
    };

    uint16_t size = (uint16_t)(radius * (0.25f * 0xFFFF));

    renderable->point_render.particles = (render_batch_particles_t){
        .particles = &renderable->point_render.particle_data,
        .particle_count = 2,
        .particle_size = WORLD_SCALE * 4,
        .particle_scale_width = size,
        .particle_scale_height = size,
    };

    renderable->point_render.material = material_cache_load(material_filename);
}

void renderable_destroy_direct(struct renderable* renderable) {
    free(renderable->mesh_render.attachments);
    renderable->mesh_render.mesh = NULL;
}

void renderable_destroy(struct renderable* renderable) {
    rspq_call_deferred((void (*)(void*))tmesh_cache_release, renderable->mesh_render.mesh);
    renderable_destroy_direct(renderable);
}

// released with renderable_destroy()
void renderable_single_axis_init(struct renderable* renderable, struct TransformSingleAxis* transform, const char* mesh_filename) {
    transform_mixed_init_sa(&renderable->transform, transform);
    renderable->mesh_render.mesh = tmesh_cache_load(mesh_filename);
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

void renderable_single_axis_init_direct(struct renderable* renderable, struct TransformSingleAxis* transform, struct tmesh* mesh) {
    transform_mixed_init_sa(&renderable->transform, transform);
    renderable->mesh_render.mesh = mesh;
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

void renderable_destroy_point(struct renderable* renderable) {
    material_cache_release(renderable->point_render.material);
}