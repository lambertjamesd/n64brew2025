#include "armature.h"

#include <malloc.h>
#include <memory.h>
#include <math.h>
#include "../math/matrix.h"
#include "../util/flags.h"
#include "../render/defs.h"

#define POSITION_SCALE      (WORLD_SCALE / 256.0f)
#define QUATERNION_SCALE    (1.0f / 32767.0f)
#define SCALE_SCALE         (1.0f / 256.0f)

void armature_unpack_transform(struct armature_packed_transform* packed, struct Transform* result) {
    result->position.x = packed->x * POSITION_SCALE;
    result->position.y = packed->y * POSITION_SCALE;
    result->position.z = packed->z * POSITION_SCALE;

    result->rotation.x = packed->rx * QUATERNION_SCALE;
    result->rotation.y = packed->ry * QUATERNION_SCALE;
    result->rotation.z = packed->rz * QUATERNION_SCALE;

    float wSqrd = 1.0f - result->rotation.x * result->rotation.x - result->rotation.y * result->rotation.y - result->rotation.z * result->rotation.z;
    result->rotation.w = wSqrd > 0.0f ? sqrtf(wSqrd) : 0.0f;

    result->scale.x = packed->sx * SCALE_SCALE;
    result->scale.y = packed->sy * SCALE_SCALE;
    result->scale.z = packed->sz * SCALE_SCALE;
}

void armature_definition_init(struct armature_definition* definition, int bone_count) {
    definition->bone_count = bone_count;

    if (bone_count) {
        definition->parent_linkage = malloc(sizeof(uint8_t) * bone_count);
        definition->default_pose = malloc(sizeof(struct armature_packed_transform) * bone_count);
    } else {
        definition->parent_linkage = 0;
        definition->default_pose = 0;
    }

    definition->image_frames_0 = 0;
    definition->image_frames_1 = 0;

    definition->frames = NULL;

    definition->flags = 0;
}

void armature_definition_destroy(struct armature_definition* definition) {
    free(definition->default_pose);
    free(definition->parent_linkage);
    definition->default_pose = 0;
    definition->parent_linkage = 0;
}

void armature_init(struct armature* armature, struct armature_definition* definition) {
    armature->bone_count = definition ? definition->bone_count : 0;
    armature->definition = definition;
    armature->image_frame_0 = NO_IMAGE_FRAME;
    armature->image_frame_1 = NO_IMAGE_FRAME;

    armature->has_prim_color = HAS_FLAG(definition->flags, ARMATURE_DEF_FLAGS_HAS_PRIM);
    armature->has_env_color = HAS_FLAG(definition->flags, ARMATURE_DEF_FLAGS_HAS_ENV);

    if (armature->bone_count) {
        armature->pose = malloc(sizeof(struct Transform) * definition->bone_count);

        for (int i = 0; i < definition->bone_count; i += 1) {
            armature_unpack_transform(&definition->default_pose[i], &armature->pose[i]);
        }
    } else {
        armature->pose = 0;
    }
}

void armature_destroy(struct armature* armature) {
    free(armature->pose);
    armature->pose = 0;
}

void armature_def_apply(struct armature_definition* definition, T3DMat4FP* pose) {
    if (!definition->bone_count) {
        return;
    }

    struct Transform fullTransforms[definition->bone_count];

    for (int i = 0; i < definition->bone_count; i += 1) {
        int parent = definition->parent_linkage[i];

        if (parent == ARM_NO_PARENT_LINK) {
            armature_unpack_transform(&definition->default_pose[i], &fullTransforms[i]);
        } else {
            struct Transform unpacked;
            armature_unpack_transform(&definition->default_pose[i], &unpacked);
            transformConcat(&fullTransforms[parent], &unpacked, &fullTransforms[i]);
        }

        T3DMat4 mtx;
        transformToMatrix(&fullTransforms[i], mtx.m);
        t3d_mat4_to_fixed(&pose[i], &mtx);
    }

    data_cache_hit_writeback_invalidate(pose, sizeof(T3DMat4FP) * definition->bone_count);
}

T3DMat4* armature_build_pose(struct armature* armature, struct frame_memory_pool* pool) {
    T3DMat4* pose = frame_malloc(pool, sizeof(T3DMat4) * armature->bone_count);

    if (!pose) {
        return NULL;
    }

    uint8_t* parent_linkage = armature->definition->parent_linkage;

    for(int i = 0; i < armature->bone_count; i++) {
        int parent_index = parent_linkage[i];

        assert(parent_index == NO_BONE_PARENT || parent_index < i);

        if (parent_index == NO_BONE_PARENT) {
            transformToMatrix(&armature->pose[i], pose[i].m);
        } else {
            mat4x4 tmp;
            transformToMatrix(&armature->pose[i], tmp);
            matrixMul(pose[parent_index].m, tmp, pose[i].m);
        }
    }

    return pose;
}

void armature_bone_transform(struct armature* armature, int bone_index, struct Transform* result) {
    transformInitIdentity(result);

    uint8_t* parent_linkage = armature->definition->parent_linkage;

    while (bone_index >= 0 && bone_index < armature->bone_count) {
        struct Transform tmp;
        transformConcat(&armature->pose[bone_index], result, &tmp);
        *result = tmp;

        bone_index = parent_linkage[bone_index];
    }
}