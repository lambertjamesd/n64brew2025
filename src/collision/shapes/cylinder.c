#include "cylinder.h"

#include "../dynamic_object.h"
#include "../../math/constants.h"
#include <math.h>

void cylinder_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float abs_x = fabsf(direction->x);
    float abs_z = fabsf(direction->z);
    float angle_dot = (abs_x + abs_z) * SQRT_1_2;
    bool use_angle = false;

    if (abs_x < angle_dot && abs_z < angle_dot) {
        output->x = direction->x > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
        output->z = direction->z > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
    } else if (abs_x > abs_z) {
        output->x = direction->x > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
        output->z = 0.0f;
    } else {
        output->x = 0.0f;
        output->z = direction->z > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
    }

    output->y = direction->y > 0.0f ? shape_data->cylinder.half_height : -shape_data->cylinder.half_height;
}

void cylinder_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    box->min.x = -shape_data->cylinder.radius;
    box->min.y = -shape_data->cylinder.half_height;
    box->min.z = -shape_data->cylinder.radius;

    box->max.x = shape_data->cylinder.radius;
    box->max.y = shape_data->cylinder.half_height;
    box->max.z = shape_data->cylinder.radius;
}