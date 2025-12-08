#include "renderable.h"

#include "../resource/tmesh_cache.h"
#include <stddef.h>

void _renderable_init(struct renderable* renderable) {
    renderable->force_material = NULL;
    renderable->attrs = NULL;
    armature_init(&renderable->armature, &renderable->mesh->armature);

    if (renderable->mesh->attatchment_count) {
        renderable->attachments = malloc(sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
        memset(renderable->attachments, 0, sizeof(struct tmesh*) * renderable->mesh->attatchment_count);
    } else {
        renderable->attachments = NULL;
    }
    renderable->hide = 0;
}

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename) {
    transform_mixed_init(&renderable->transform, transform);
    renderable->mesh = tmesh_cache_load(mesh_filename);
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_BASIC;
}

void renderable_destroy_direct(struct renderable* renderable) {
    free(renderable->attachments);
    renderable->mesh = NULL;
}

void renderable_destroy(struct renderable* renderable) {
    rspq_call_deferred((void (*)(void*))tmesh_cache_release, renderable->mesh);
    renderable_destroy_direct(renderable);
}

// released with renderable_destroy()
void renderable_single_axis_init(struct renderable* renderable, struct TransformSingleAxis* transform, const char* mesh_filename) {
    transform_mixed_init_sa(&renderable->transform, transform);
    renderable->mesh = tmesh_cache_load(mesh_filename);
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

void renderable_single_axis_init_direct(struct renderable* renderable, struct TransformSingleAxis* transform, struct tmesh* mesh) {
    transform_mixed_init_sa(&renderable->transform, transform);
    renderable->mesh = mesh;
    _renderable_init(renderable);
    renderable->type = TRANSFORM_TYPE_SINGLE_AXIS;
}