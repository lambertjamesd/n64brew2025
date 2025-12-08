#include "cylinder.h"

#include "../dynamic_object.h"
#include "../../math/minmax.h"
#include <math.h>

void cone_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    output->x = direction->x > 0.0f ? shape_data->cone.size.x : -shape_data->cone.size.x;
    output->y = direction->y > 0.0f ? shape_data->cone.size.y : -shape_data->cone.size.y;
    output->z = shape_data->cone.size.z;

    if (vector3Dot(output, direction) < 0) {
        *output = gZeroVec;
    }
}

void cone_bounding_box(void* data, struct Vector2* rotation, struct Box3D* box) {
    union dynamic_object_type_data* shape_data = (union dynamic_object_type_data*)data;

    struct Vector2 corner;

    corner.x = shape_data->cone.size.x;
    corner.y = shape_data->cone.size.z;

    struct Vector2 cornerRotated;
    vector2ComplexMul(&corner, rotation, &cornerRotated);

    box->min.x = cornerRotated.x;
    box->min.y = -shape_data->cone.size.y;
    box->min.z = cornerRotated.y;

    box->max = box->min;
    box->max.y = shape_data->cone.size.y;

    corner.x = -corner.x;
    vector2ComplexMul(&corner, rotation, &cornerRotated);

    box->min.x = MIN(box->min.x, cornerRotated.x);
    box->min.z = MIN(box->min.z, cornerRotated.y);

    box->max.x = MAX(box->max.x, cornerRotated.x);
    box->max.z = MAX(box->max.z, cornerRotated.y);

    box->min.x = MIN(box->min.x, 0.0f);
    box->min.z = MIN(box->min.z, 0.0f);

    box->max.x = MAX(box->max.x, 0.0f);
    box->max.z = MAX(box->max.z, 0.0f);
}