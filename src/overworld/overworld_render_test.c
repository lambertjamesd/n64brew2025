#include "overworld_render.h"
#include "../test/framework_test.h"

#include "overworld_private.h"

void test_overworld_create_top_view(struct test_context* t) {
    struct overworld overworld = {
        .min = {0.0f, 0.0f},
        .inv_tile_size = 1.0f,
    };

    mat4x4 projMatrix;
    matrixPerspective(projMatrix, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 4.0f);

    struct Vector2 loop[8];
    int loop_count = overworld_create_top_view(&overworld, projMatrix, &gZeroVec, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -4.0f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -4.0f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){1.0f, -1.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-1.0f, -1.0f}), &loop[3]);

    mat4x4 rotateMatrix;
    struct Quaternion rotation;
    quatAxisAngle(&gUp, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);

    mat4x4 combinedMatrix;
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, &gZeroVec, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){0.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){5.65685606f, 0.0f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){1.41421366f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){0.0f, -1.41421366f}), &loop[3]);

    quatAxisAngle(&gRight, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, &gZeroVec, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.65685606f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, 0.0f}), &loop[3]);

    quatAxisAngle(&gRight, 0.785398163f, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, &gZeroVec, loop);
    test_eqi(t, 4, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.65685606f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.65685606f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, 0.0f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, 0.0f}), &loop[3]);

    quatAxisAngle(&gRight, 0.392699081, &rotation);
    quatToMatrix(&rotation, rotateMatrix);
    matrixMul(projMatrix, rotateMatrix, combinedMatrix);

    loop_count = overworld_create_top_view(&overworld, combinedMatrix, &gZeroVec, loop);
    test_eqi(t, 6, loop_count);

    test_vec2_equal(t, (&(struct Vector2){-4.0f, -5.22625303f}), &loop[0]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -5.22625303f}), &loop[1]);
    test_vec2_equal(t, (&(struct Vector2){4.0f, -2.16478419f}), &loop[2]);
    test_vec2_equal(t, (&(struct Vector2){1.0f, -0.541196108f}), &loop[3]);
    test_vec2_equal(t, (&(struct Vector2){-1.0f, -0.541196108f}), &loop[4]);
    test_vec2_equal(t, (&(struct Vector2){-4.0f, -2.16478419f}), &loop[5]);
}

void test_overworld_step(struct test_context* t) {
    struct overworld_step_state state = {
        .loop = {
            {0.25f, 0.5f},
            {5.25f, 0.5f},
            {9.25f, 4.5f},
            {-3.75f, 4.5f},
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f},
        },
        .loop_count = 4,
        .left = 0,
        .right = 0,
        .current_y = 0.5f,
        .min_x = 0.5f,
        .max_x = 0.5f,
    };

    struct overworld overworld = {
        .tile_x = 10,
        .tile_y = 10,
    };

    struct overworld_tile_slice slice = overworld_step(&overworld, &state);
    test_eqi(t, 0, slice.min_x);
    test_eqi(t, 6, slice.max_x);
    test_eqi(t, 0, slice.y);
    test_eqi(t, 1, slice.has_more);

    slice = overworld_step(&overworld, &state);
    test_eqi(t, 0, slice.min_x);
    test_eqi(t, 7, slice.max_x);
    test_eqi(t, 1, slice.y);
    test_eqi(t, 1, slice.has_more);

    slice = overworld_step(&overworld, &state);
    test_eqi(t, 0, slice.min_x);
    test_eqi(t, 8, slice.max_x);
    test_eqi(t, 2, slice.y);
    test_eqi(t, 1, slice.has_more);

    slice = overworld_step(&overworld, &state);
    test_eqi(t, 0, slice.min_x);
    test_eqi(t, 9, slice.max_x);
    test_eqi(t, 3, slice.y);
    test_eqi(t, 1, slice.has_more);

    slice = overworld_step(&overworld, &state);
    test_eqi(t, 0, slice.min_x);
    test_eqi(t, 9, slice.max_x);
    test_eqi(t, 4, slice.y);
    test_eqi(t, 0, slice.has_more);
}