#ifndef __MATH_TRANSFORM_MIXED_H__
#define __MATH_TRANSFORM_MIXED_H__

#include "vector3.h"
#include "transform.h"
#include "transform_single_axis.h"

#include <stdbool.h>

enum transform_type {
    TRANSFORM_TYPE_BASIC,
    TRANSFORM_TYPE_SINGLE_AXIS,
    TRANSFORM_TYPE_POSITION,
};

struct transform_mixed {
    void* transform;
    enum transform_type type;
};

void transform_mixed_init(struct transform_mixed* transform_mixed, struct Transform* transform);
void transform_mixed_init_sa(struct transform_mixed* transform_mixed, struct TransformSingleAxis* transform);
void transform_mixed_init_pos(struct transform_mixed* transform_mixed, struct Vector3* transform);

struct Vector3* transform_mixed_get_position(struct transform_mixed* transform);

bool transform_rotate_towards(struct TransformSingleAxis* transform, struct Vector3* forward, float max_radians);

#endif