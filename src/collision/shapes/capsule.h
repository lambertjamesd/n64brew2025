#ifndef __COLLISION_SHAPE_CAPSULE_H__
#define __COLLISION_SHAPE_CAPSULE_H__

#include "../../math/vector2.h"
#include "../../math/vector3.h"
#include "../../math/box3d.h"

void capsule_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void capsule_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

#define CAPSULE_COLLIDER(radius, inner_half_height) \
.minkowsi_sum = capsule_minkowski_sum, \
.bounding_box = capsule_bounding_box,  \
.data = {                              \
    .capsule = {                       \
        (radius),                      \
        (inner_half_height),           \
    },                                 \
}

#endif