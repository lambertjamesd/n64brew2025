#include "cylinder.h"

#include "../dynamic_object.h"
#include <math.h>
#include "sphere.h"

void capsule_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;
    sphere_minkowski_sum(data, direction, output);

    if (direction->y > 0.0f) {
        output->y += shape_data->capsule.inner_half_height;
    } else {
        output->y -= shape_data->capsule.inner_half_height;
    }
}

void capsule_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    vector3Scale(&gOneVec, &box->max, shape_data->capsule.radius);
    vector3Scale(&gOneVec, &box->min, -shape_data->capsule.radius);

    box->max.y += shape_data->capsule.inner_half_height;
    box->min.y -= shape_data->capsule.inner_half_height;
}