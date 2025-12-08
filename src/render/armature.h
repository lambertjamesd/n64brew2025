#ifndef __RENDER_ARMATURE_H__
#define __RENDER_ARMATURE_H__

#include <stdint.h>
#include <t3d/t3d.h>
#include "../math/vector3.h"
#include "../math/transform.h"
#include "armature_definition.h"
#include "../render/frame_alloc.h"

#define NO_BONE_PARENT  0xFF
#define NO_IMAGE_FRAME  0xFF

struct armature {
    struct armature_definition* definition;
    struct Transform* pose;
    uint16_t bone_count;
    uint8_t image_frame_0;
    uint8_t image_frame_1;
    // frames can trigger events
    uint16_t active_events;
    uint16_t has_prim_color: 1;
    uint16_t has_env_color: 1;
    color_t prim_color;
    color_t env_color;
};

void armature_definition_init(struct armature_definition* definition, int boune_count);
void armature_definition_destroy(struct armature_definition* definition);

void armature_init(struct armature* armature, struct armature_definition* definition);
void armature_destroy(struct armature* armature);

// this wont be needed once instancing is working
void armature_def_apply(struct armature_definition* definition, T3DMat4FP* pose);

T3DMat4* armature_build_pose(struct armature* armature, struct frame_memory_pool* pool);

void armature_bone_transform(struct armature* armature, int bone_index, struct Transform* result);

#endif