#ifndef __SHAPES_SWING_SHAPE_H__
#define __SHAPES_SWING_SHAPE_H__

#include "../../math/vector3.h"
#include "../../math/vector2.h"
#include "../../math/box3d.h"
#include <stdint.h>

struct swing_shape {
    struct Vector3 corner[4];
    uint8_t next_corner;
    uint8_t data_count;
};

void swing_shape_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void swing_shape_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box);

void swing_shape_init(struct swing_shape* shape);
void swing_shape_add(struct swing_shape* shape, struct Vector3* a, struct Vector3* b);


#endif