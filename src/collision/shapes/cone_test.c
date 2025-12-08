#include "cone.h"

#include "../../test/framework_test.h"
#include "../dynamic_object.h"
#include "../../math/constants.h"
#include <libdragon.h>

void test_cone_minkowski_sum(struct test_context* t) {
    union dynamic_object_type_data data = {
        .cone = {
            .size = { 1.0f, 1.0f, 2.0f },
        },
    };
    struct Vector3 directon;
    struct Vector3 output;

    directon = (struct Vector3){1.0f, 1.0f, 1.0f};
    cone_minkowski_sum(&data, &directon, &output);
    test_near_equalf(t, 1.0f, output.x);
    test_near_equalf(t, 1.0f, output.y);
    test_near_equalf(t, 2.0f, output.z);

    directon = (struct Vector3){-1.0f, 1.0f, 1.0f};
    cone_minkowski_sum(&data, &directon, &output);
    test_near_equalf(t, -1.0f, output.x);
    test_near_equalf(t, 1.0f, output.y);
    test_near_equalf(t, 2.0f, output.z);

    directon = (struct Vector3){1.0f, 1.0f, -2.0f};
    cone_minkowski_sum(&data, &directon, &output);
    test_near_equalf(t, 0.0f, output.x);
    test_near_equalf(t, 0.0f, output.y);
    test_near_equalf(t, 0.0f, output.z);
}

void test_cone_bounding_box(struct test_context* t) {
    union dynamic_object_type_data data = {
        .cone = {
            .size = { 1.0f, 1.0f, 2.0f },
        },
    };
    struct Vector2 rotation = { 1.0f, 0.0f };

    struct Box3D bb;
    cone_bounding_box(&data, &rotation, &bb);
    test_near_equalf(t, -1.0f, bb.min.x);
    test_near_equalf(t, -1.0f, bb.min.y);
    test_near_equalf(t, 0.0f, bb.min.z);
    test_near_equalf(t, 1.0f, bb.max.x);
    test_near_equalf(t, 1.0f, bb.max.y);
    test_near_equalf(t, 2.0f, bb.max.z);

    vector2ComplexFromAngle(M_PI * 0.25f, &rotation);
    cone_bounding_box(&data, &rotation, &bb);
    test_near_equalf(t, 0.0f, bb.min.x);
    test_near_equalf(t, -1.0f, bb.min.y);
    test_near_equalf(t, 0.0f, bb.min.z);
    test_near_equalf(t, 2.12132049f, bb.max.x);
    test_near_equalf(t, 1.0f, bb.max.y);
    test_near_equalf(t, 2.12132049f, bb.max.z);
}