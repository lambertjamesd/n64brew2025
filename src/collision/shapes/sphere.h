#ifndef __COLLISION_SHAPE_SPHERE_H__
#define __COLLISION_SHAPE_SPHERE_H__

#include "../../math/vector2.h"
#include "../../math/vector3.h"
#include "../../math/box3d.h"

void sphere_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void sphere_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

#define SPHERE_COLLIDER(radius) \
    .minkowsi_sum = sphere_minkowski_sum, \
    .bounding_box = sphere_bounding_box,  \
    .data = {                               \
        .sphere = {                       \
            (radius),                       \
        },                                  \
    }

#endif