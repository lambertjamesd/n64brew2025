#include "swing_shape.h"

#include "../dynamic_object.h"
#include <memory.h>

#define COLLIDER_DROP   0.5f

void swing_shape_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;
    struct swing_shape* shape = shape_data->swing.shape;

    if (shape->data_count == 0) {
        *output = gZeroVec;
        return;
    }

    *output = shape->corner[0];
    struct Vector3* result = &shape->corner[0];
    float score = vector3Dot(&shape->corner[0], direction);

    struct Vector3* max = shape->corner + shape->data_count;

    for (struct Vector3* curr = &shape->corner[1]; curr < max; curr += 1) {
        float test = vector3Dot(curr, direction);

        if (test > score) {
            score = test;
            result = curr;
        }
    }

    *output = *result;

    if (direction->y < 0.0f) {
        output->y -= COLLIDER_DROP;
    }
}

void swing_shape_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;
    struct swing_shape* shape = shape_data->swing.shape;

    if (shape->data_count == 0) {
        box->min = gZeroVec;
        box->max = gZeroVec;
        return;
    }

    box->min = shape->corner[0];
    box->max = shape->corner[0];

    struct Vector3* max = shape->corner + shape->data_count;

    for (struct Vector3* curr = &shape->corner[1]; curr < max; curr += 1) {
        vector3Min(&box->min, curr, &box->min);
        vector3Max(&box->max, curr, &box->max);
    }

    box->min.y -= COLLIDER_DROP;
}

void swing_shape_init(struct swing_shape* shape) {
    memset(shape, 0, sizeof(struct swing_shape));
}

void swing_shape_add(struct swing_shape* shape, struct Vector3* a, struct Vector3* b) {
    shape->corner[shape->next_corner+0] = *a;
    shape->corner[shape->next_corner+1] = *b;

    if (shape->data_count < 4) {
        shape->data_count += 2;
    }
    shape->next_corner ^= 2;
}