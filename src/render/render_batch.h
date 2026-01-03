#ifndef __RENDER_RENDER_BATCH_H__
#define __RENDER_RENDER_BATCH_H__

#include <libdragon.h>
#include <t3d/t3d.h>
#include <t3d/tpx.h>

#include "tmesh.h"
#include "armature.h"
#include "material.h"
#include "frame_alloc.h"

#include "../math/matrix.h"
#include "../math/transform.h"
#include "../math/transform_single_axis.h"

#define RENDER_BATCH_MAX_SIZE   256
#define RENDER_BATCH_TRANSFORM_COUNT    64
#define MAX_BILLBOARD_SPRITES           128

enum render_batch_type {
    RENDER_BATCH_MESH,
    RENDER_BATCH_PARTICLES,
    RENDER_BATCH_CALLBACK,
};

struct render_batch_particle_size {
    // particle_size = WORLD_SCALE is a radius of 1
    // when particle_scale = 0xFFFF and particle_size = 127
    uint16_t particle_size;
    uint16_t particle_scale_width;
    uint16_t particle_scale_height;
};

struct render_batch_particles {
    TPXParticle* particles;
    uint16_t particle_count;
    // particle_size = WORLD_SCALE is a radius of 1
    // when particle_scale = 0xFFFF and particle_size = 127
    uint16_t particle_size;
    uint16_t particle_scale_width;
    uint16_t particle_scale_height;
};

typedef struct render_batch_particles render_batch_particles_t;

struct render_batch;

typedef void (*RenderCallback)(void* data, struct render_batch* batch);

enum element_attr_type {
    ELEMENT_ATTR_NONE,
    ELEMENT_ATTR_POSE,
    ELEMENT_ATTR_IMAGE,
    ELEMENT_ATTR_PRIM_COLOR,
    ELEMENT_ATTR_ENV_COLOR,
    ELEMENT_ATTR_TRANSFORM,
    ELEMENT_ATTR_TRANSFORM_LIST,
    ELEMENT_ATTR_SCROLL,
};

struct element_attr {
    uint8_t type;
    uint8_t offset;
    union {
        struct {
            T3DMat4FP* pose;
        } pose;
        struct {
            sprite_t* sprite;
        } image;
        color_t color;
        T3DMat4FP* transform;
        T3DMat4FP** transform_list;
        struct {
            int16_t x, y;
        } scroll;
    };
};

typedef struct element_attr element_attr_t;

static inline element_attr_t element_attr_prim_color(color_t color) {
    return (element_attr_t) {
        .type = ELEMENT_ATTR_PRIM_COLOR,
        .color = color,
    };
}

static inline element_attr_t element_attr_end() {
    return (element_attr_t){.type = ELEMENT_ATTR_NONE};
}

struct render_batch_element {
    struct material* material;
    uint16_t type;
    uint8_t light_source;
    union {
        struct {
            rspq_block_t* block;
            struct element_attr* attrs;
        } mesh;
        struct {
            render_batch_particles_t* particles;
            T3DMat4FP* transform;
        } particles;
        struct {
            RenderCallback callback;
            void* data;
        } callback;
    };
};

typedef struct render_batch_element render_batch_element_t;

struct render_batch {
    mat4x4 camera_matrix;
    struct Vector2 rotation_2d;
    struct frame_memory_pool* pool;
    struct render_batch_element elements[RENDER_BATCH_MAX_SIZE];
    short element_count;
};

typedef struct render_batch render_batch_t;

void render_batch_init(struct render_batch* batch, struct Transform* camera_transform, struct frame_memory_pool* pool);

struct render_batch_element* render_batch_add(struct render_batch* batch);

struct render_batch_element* render_batch_add_tmesh(
    struct render_batch* batch, 
    struct tmesh* mesh, 
    T3DMat4FP* transform,
    struct armature* armature, 
    struct tmesh** attachments,
    struct element_attr* additional_attrs
);

void render_batch_add_callback(struct render_batch* batch, struct material* material, RenderCallback callback, void* data);

struct render_batch_element* render_batch_add_particles(
    struct render_batch* batch, 
    struct material* material, 
    render_batch_particles_t* particles, 
    T3DMat4FP* mtx
);

struct render_batch_element* render_batch_add_dynamic_particles(
    struct render_batch* batch, 
    struct material* material, 
    int count, 
    const struct render_batch_particle_size* size,
    T3DMat4FP* mtx
);

mat4x4* render_batch_get_transform(struct render_batch* batch);
T3DMat4FP* render_batch_get_transformfp(struct render_batch* batch);
T3DMat4FP* render_batch_transformfp_from_sa(struct render_batch* batch, struct TransformSingleAxis* transform);
T3DMat4FP* render_batch_transformfp_from_full(struct render_batch* batch, struct Transform* transform);
// !!! This stomps on the input pose so don't attempt to use it after calling this function
T3DMat4FP* render_batch_build_pose(T3DMat4* pose, int bone_count);

void render_batch_relative_mtx(struct render_batch* batch, mat4x4 into);

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj, T3DViewport* viewport);

#endif