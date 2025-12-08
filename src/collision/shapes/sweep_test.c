#include "sweep.h"
#include "../../test/framework_test.h"
#include "../dynamic_object.h"
#include "../dynamic_object_test.h"

static union dynamic_object_type_data test_sweep_data = {
    .sweep = {
        .range = {0.707106781f, 0.707106781f},
        .radius = 2.0f,
        .half_height = 0.25f,
    }
};

void test_sweep_minkowski_sum(struct test_context* t) {
    struct Vector3 result;

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){0.0f, 0.1f, 1.0f},
        &result
    );
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, test_sweep_data.sweep.radius, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){1.0f, -0.1f, 0.0f},
        &result
    );
    test_near_equalf(t, test_sweep_data.sweep.radius * test_sweep_data.sweep.range.x, result.x);
    test_near_equalf(t, -test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, test_sweep_data.sweep.radius * test_sweep_data.sweep.range.y, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){-0.8f, 0.1f, 1.707f},
        &result
    );
    test_near_equalf(t, -test_sweep_data.sweep.radius * test_sweep_data.sweep.range.x, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, test_sweep_data.sweep.radius * test_sweep_data.sweep.range.y, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){0.6f, 0.1f, 1.707f},
        &result
    );
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, test_sweep_data.sweep.radius, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){0.0f, 0.1f, -1.707f},
        &result
    );
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, 0.0f, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){0.707f, 0.1f, -0.717f},
        &result
    );
    test_near_equalf(t, 0.0f, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, 0.0f, result.z);

    sweep_minkowski_sum(
        &test_sweep_data, 
        &(struct Vector3){0.717f, 0.1f, -0.707f},
        &result
    );
    test_near_equalf(t, test_sweep_data.sweep.radius * test_sweep_data.sweep.range.x, result.x);
    test_near_equalf(t, test_sweep_data.sweep.half_height, result.y);
    test_near_equalf(t, test_sweep_data.sweep.radius * test_sweep_data.sweep.range.y, result.z);
}

void test_sweep_bounding_box(struct test_context* t) {
    test_verify_bb_calculator(t, &test_sweep_data, sweep_bounding_box, sweep_minkowski_sum);
}