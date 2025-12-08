#include "sphere.h"

#include "../dynamic_object.h"
#include <math.h>

#define SQRT_1_3  0.577350269f

void sphere_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    float distance = fabsf(direction->x);
    int axis_index = 0;

    for (int i = 1; i < 3; ++i) {
        float distanceCheck = fabsf(VECTOR3_AS_ARRAY(direction)[i]);

        if (distanceCheck > distance) {
            distance = distanceCheck;
            axis_index = i;
        }
    }

    *output = gZeroVec;
    if (VECTOR3_AS_ARRAY(direction)[axis_index] > 0.0f) {
        VECTOR3_AS_ARRAY(output)[axis_index] = shape_data->sphere.radius;;
    } else {
        VECTOR3_AS_ARRAY(output)[axis_index] = -shape_data->sphere.radius;;
    }

    // float distanceCheck = (fabsf(direction->x) + fabsf(direction->y) + fabsf(direction->z)) * SQRT_1_3;

    // if (distanceCheck > distance) {
    //     float scaledRadius = radius * SQRT_1_3;

    //     if (output->x > 0.0f) {
    //         output->x = scaledRadius;
    //     } else {
    //         output->x = -scaledRadius;
    //     }

    //     if (output->y > 0.0f) {
    //         output->y = scaledRadius;
    //     } else {
    //         output->y = -scaledRadius;
    //     }

    //     if (output->z > 0.0f) {
    //         output->z = scaledRadius;
    //     } else {
    //         output->z = -scaledRadius;
    //     }
    // }
}

void sphere_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    vector3Scale(&gOneVec, &box->max, shape_data->sphere.radius);
    vector3Scale(&gOneVec, &box->min, -shape_data->sphere.radius);
}