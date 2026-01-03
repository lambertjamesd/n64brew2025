#ifndef __RENDER_TMESH_H__
#define __RENDER_TMESH_H__

#include <libdragon.h>
#include <t3d/t3d.h>

#include "material.h"
#include "armature.h"
#include "../math/transform.h"

enum light_source {
    LIGHT_SOURCE_NONE,
    LIGHT_SOURCE_CAMERA,
    LIGHT_SOURCE_RIM,
};

struct armature_attatchment {
    char* name;
    uint16_t bone_index;
    T3DMat4FP local_transform;
};

struct tmesh {
    struct material* material;
    struct material* transition_materials;
    rspq_block_t* block;
    T3DVertPacked* vertices;
    uint16_t vertex_count;
    uint16_t material_transition_count;
    struct armature_definition armature;

    struct armature_attatchment* attatchments;
    uint16_t attatchment_count;
    uint8_t light_source;
};

typedef struct tmesh tmesh_t;

void tmesh_load(struct tmesh* tmesh, FILE* file);
void tmesh_load_filename(struct tmesh* tmesh, const char* filename);
void tmesh_release(struct tmesh* tmesh);

#endif