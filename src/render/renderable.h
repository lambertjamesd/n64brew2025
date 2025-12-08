#ifndef __RENDER_RENDERABLE_H__
#define __RENDER_RENDERABLE_H__

#include "../math/transform.h"
#include "../math/transform_single_axis.h"
#include "../math/transform_mixed.h"
#include "../render/render_batch.h"
#include "armature.h"

struct renderable {
    struct transform_mixed transform;
    struct tmesh* mesh;
    struct armature armature;
    struct material* force_material;
    struct tmesh** attachments;
    enum transform_type type;
    struct element_attr* attrs;
    uint16_t hide: 1;
};

typedef struct renderable renderable_t;

void renderable_init(struct renderable* renderable, struct Transform* transform, const char* mesh_filename);
void renderable_single_axis_init(struct renderable* renderable, struct TransformSingleAxis* transform, const char* mesh_filename);

void renderable_single_axis_init_direct(struct renderable* renderable, struct TransformSingleAxis* transform, struct tmesh* mesh);

void renderable_destroy(struct renderable* renderable);
void renderable_destroy_direct(struct renderable* renderable);


#endif