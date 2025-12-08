#include "sphere.h"

#include "../../test/framework_test.h"
#include "../dynamic_object.h"

void test_sphere_minkowski_sum(struct test_context* t) {
    struct Vector3 offset;
    struct Vector3 result;

    union dynamic_object_type_data data = {
        .sphere.radius = 2.0f,
    };

    offset = (struct Vector3){.x = 1.0f, .y = 0.0f, .z = 0.0f};
    sphere_minkowski_sum(&data, &offset, &result);
    test_near_equalf(t, 2.0f, result.x);
    test_near_equalf(t, 0.0f, result.y);
    test_near_equalf(t, 0.0f, result.z);

    offset = (struct Vector3){.x = 0.0f, .y = 1.0f, .z = 0.0f};
    sphere_minkowski_sum(&data, &offset, &result);
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, 2.0f, result.y);
    test_near_equalf(t, 0.0f, result.z);

    offset = (struct Vector3){.x = 0.0f, .y = 0.0f, .z = -2.0f};
    sphere_minkowski_sum(&data, &offset, &result);
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, 0.0f, result.y);
    test_near_equalf(t, -2.0f, result.z);

    offset = (struct Vector3){.x = 0.0f, .y = 2.1f, .z = -2.0f};
    sphere_minkowski_sum(&data, &offset, &result);
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, 2.0f, result.y);
    test_near_equalf(t, 0.0f, result.z);
}