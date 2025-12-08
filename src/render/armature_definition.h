#ifndef __RENDER_ARMATURE_DEFINITION_H__
#define __RENDER_ARMATURE_DEFINITION_H__

#include <stdint.h>
#include <t3d/t3d.h>

#define ARM_NO_PARENT_LINK  0xff

#define ARMATURE_DEF_FLAGS_HAS_PRIM     (1 << 15)
#define ARMATURE_DEF_FLAGS_HAS_ENV     (1 << 14)

struct armature_packed_transform {
    int16_t x, y, z;
    int16_t rx, ry, rz; // w is derived from the other values
    int16_t sx, sy, sz;
};

struct armature_definition {
    uint8_t* parent_linkage;
    struct armature_packed_transform* default_pose;
    uint16_t bone_count;
    uint8_t image_frames_0;
    uint8_t image_frames_1;
    sprite_t** frames;
    uint16_t flags;
};

#endif