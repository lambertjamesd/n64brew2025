#include "cylinder.h"

#include "../../test/framework_test.h"
#include "../dynamic_object.h"
#include "../../math/constants.h"

void cylinder_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

void test_cylinder_minkowski_sum(struct test_context* t) {
    union dynamic_object_type_data data = {
        .cylinder.half_height = 1.0f,
        .cylinder.radius = 1.0f,
    };

    struct Vector3 direction;
    struct Vector3 output;

    direction = (struct Vector3){ 1.0f, 0.1f, 1.0f};
    cylinder_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, SQRT_1_2, output.x);
    test_near_equalf(t, 1.0f, output.y);
    test_near_equalf(t, SQRT_1_2, output.z);

    direction = (struct Vector3){ 1.0f, -1.0f, 0.0f};
    cylinder_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 1.0f, output.x);
    test_near_equalf(t, -1.0f, output.y);
    test_near_equalf(t, 0.0f, output.z);

    direction = (struct Vector3){ 0.0f, -1.0f, -1.0f};
    cylinder_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 0.0f, output.x);
    test_near_equalf(t, -1.0f, output.y);
    test_near_equalf(t, -1.0f, output.z);

    direction = (struct Vector3){ 0.923879533f + 0.1f, 0.1f, 0.382683432f};
    cylinder_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, 1.0f, output.x);
    test_near_equalf(t, 1.0f, output.y);
    test_near_equalf(t, 0.0f, output.z);

    direction = (struct Vector3){ 0.923879533f - 0.1f, 0.1f, 0.382683432f};
    cylinder_minkowski_sum(&data, &direction, &output);
    test_near_equalf(t, SQRT_1_2, output.x);
    test_near_equalf(t, 1.0f, output.y);
    test_near_equalf(t, SQRT_1_2, output.z);
}