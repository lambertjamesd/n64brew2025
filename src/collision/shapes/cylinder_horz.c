#include "cylinder_horz.h"

#include "../dynamic_object.h"
#include "../../math/constants.h"
#include <math.h>

void cylinder_horz_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float abs_x = fabsf(direction->x);
    float abs_y = fabsf(direction->y);
    float angle_dot = (abs_x + abs_y) * SQRT_1_2;
    bool use_angle = false;

    if (abs_x < angle_dot && abs_y < angle_dot) {
        output->x = direction->x > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
        output->y = direction->y > 0.0f ? SQRT_1_2 * shape_data->cylinder.radius : -SQRT_1_2 * shape_data->cylinder.radius;
    } else if (abs_x > abs_y) {
        output->x = direction->x > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
        output->y = 0.0f;
    } else {
        output->x = 0.0f;
        output->y = direction->y > 0.0f ? shape_data->cylinder.radius : -shape_data->cylinder.radius;
    }

    output->z = direction->z > 0.0f ? shape_data->cylinder.half_height : -shape_data->cylinder.half_height;
}

void cylinder_horz_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float radius = shape_data->cylinder.radius;
    float half_height = shape_data->cylinder.half_height;

    float x_offset = fabsf(rotation->x * radius) + fabsf(rotation->y * half_height);
    float z_offset = fabsf(rotation->x * half_height) + fabsf(rotation->y * radius);

    box->min.x = -x_offset;
    box->min.y = -radius;
    box->min.z = -z_offset;

    box->max.x = x_offset;
    box->max.y = radius;
    box->max.z = z_offset;
}