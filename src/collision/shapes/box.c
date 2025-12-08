#include "cylinder.h"

#include "../dynamic_object.h"
#include <math.h>

void box_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    output->x = direction->x > 0.0f ? shape_data->box.half_size.x : -shape_data->box.half_size.x;
    output->y = direction->y > 0.0f ? shape_data->box.half_size.y : -shape_data->box.half_size.y;
    output->z = direction->z > 0.0f ? shape_data->box.half_size.z : -shape_data->box.half_size.z;
}

void box_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    struct Vector3* half_size = &shape_data->box.half_size;

    if (!rotation) {
        vector3Negate(half_size, &box->min);
        box->max = *half_size;
        return;
    }

    box->max.x = half_size->x * fabsf(rotation->x) + half_size->z * fabsf(rotation->y);
    box->max.y = half_size->y;
    box->max.z = half_size->x * fabsf(rotation->y) + half_size->z * fabsf(rotation->x);

    vector3Negate(&box->max, &box->min);
}