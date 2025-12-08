#ifndef __COLLISION_SHAPE_BOX_H__
#define __COLLISION_SHAPE_BOX_H__

#include "../../math/vector2.h"
#include "../../math/vector3.h"
#include "../../math/box3d.h"

void box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void box_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

#define BOX_COLLIDER(x, y, z)          \
    .minkowsi_sum = box_minkowski_sum, \
    .bounding_box = box_bounding_box,  \
    .data = {                          \
        .box = {                       \
            .half_size = {x, y, z},    \
        },                             \
    }

#endif