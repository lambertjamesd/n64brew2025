#include "cylinder_horz.h"

#include "../../test/framework_test.h"
#include "../dynamic_object.h"
#include "../../math/constants.h"
#include <libdragon.h>

void test_cylinder_horz_minkowski_sum(struct test_context* t) {
    union dynamic_object_type_data data = {
        .cylinder.half_height = 1.0f,
        .cylinder.radius = 1.0f,
    };

    struct Vector3 direction;
    struct Vector3 output;

    direction = (struct Vector3){ 1.0f, 1.0f, 0.1f};
    cylinder_horz_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, SQRT_1_2, output.x);
    test_near_equalf(t, SQRT_1_2, output.y);
    test_near_equalf(t, 1.0f, output.z);

    direction = (struct Vector3){ 1.0f, 0.0f, -1.0f};
    cylinder_horz_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 1.0f, output.x);
    test_near_equalf(t, 0.0f, output.y);
    test_near_equalf(t, -1.0f, output.z);

    direction = (struct Vector3){ 0.0f, -1.0f, -1.0f};
    cylinder_horz_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 0.0f, output.x);
    test_near_equalf(t, -1.0f, output.y);
    test_near_equalf(t, -1.0f, output.z);

    direction = (struct Vector3){ 0.923879533f + 0.1f, 0.382683432f, 0.1f};
    cylinder_horz_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 1.0f, output.x);
    test_near_equalf(t, 0.0f, output.y);
    test_near_equalf(t, 1.0f, output.z);

    direction = (struct Vector3){ 0.923879533f - 0.1f, 0.382683432f, 0.1f};
    cylinder_horz_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, SQRT_1_2, output.x);
    test_near_equalf(t, SQRT_1_2, output.y);
    test_near_equalf(t, 1.0f, output.z);
}

void test_cylinder_horz_bounding_box(struct test_context* t) {
    union dynamic_object_type_data data = {
        .cylinder.half_height = 1.0f,
        .cylinder.radius = 1.0f,
    };

    struct Box3D bb;
    struct Vector2 rotation;

    for (int i = 0; i < 4; i += 1) {
        vector2ComplexFromAngle(i * M_PI * 0.5f, &rotation);
        cylinder_horz_bounding_box(&data, &rotation, &bb);
        test_near_equalf(t, -1.0f, bb.min.x);
        test_near_equalf(t, -1.0f, bb.min.y);
        test_near_equalf(t, -1.0f, bb.min.z);
    
        test_near_equalf(t, 1.0f, bb.max.x);
        test_near_equalf(t, 1.0f, bb.max.y);
        test_near_equalf(t, 1.0f, bb.max.z);
    }

    vector2ComplexFromAngle(M_PI * 0.25f, &rotation);
    cylinder_horz_bounding_box(&data, &rotation, &bb);
    test_near_equalf(t, -SQRT_1_2 * 2.0f, bb.min.x);
    test_near_equalf(t, -1.0f, bb.min.y);
    test_near_equalf(t, -SQRT_1_2 * 2.0f, bb.min.z);

    test_near_equalf(t, SQRT_1_2 * 2.0f, bb.max.x);
    test_near_equalf(t, 1.0f, bb.max.y);
    test_near_equalf(t, SQRT_1_2 * 2.0f, bb.max.z);

    data.cylinder.half_height = 2.0f;
    vector2ComplexFromAngle(0.0f, &rotation);
    cylinder_horz_bounding_box(&data, &rotation, &bb);
    test_near_equalf(t, -1.0f, bb.min.x);
    test_near_equalf(t, -1.0f, bb.min.y);
    test_near_equalf(t, -2.0f, bb.min.z);

    test_near_equalf(t, 1.0f, bb.max.x);
    test_near_equalf(t, 1.0f, bb.max.y);
    test_near_equalf(t, 2.0f, bb.max.z);

    vector2ComplexFromAngle(M_PI * 0.5f, &rotation);
    cylinder_horz_bounding_box(&data, &rotation, &bb);
    test_near_equalf(t, -2.0f, bb.min.x);
    test_near_equalf(t, -1.0f, bb.min.y);
    test_near_equalf(t, -1.0f, bb.min.z);

    test_near_equalf(t, 2.0f, bb.max.x);
    test_near_equalf(t, 1.0f, bb.max.y);
    test_near_equalf(t, 1.0f, bb.max.z);
}