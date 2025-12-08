#include "matrix.h"
#include "../test/framework_test.h"
#include <math.h>
#include <stdio.h>

void test_mtx_equal_raw(struct test_context* t, mat4x4 a, mat4x4 b, const char* location) {
    bool are_equal = true;

    for (int y = 0; y < 4; y += 1) {
        for (int x = 0; x < 4; x += 1) {
            if (fabsf(a[y][x] - b[y][x]) > 0.00001f) {
                are_equal = false;
            }
        }
    }

    if (are_equal) {
        return;
    }

    fprintf(
        stderr, 
        "| %8.4f %8.4f %8.4f %8.4f |    | %8.4f %8.4f %8.4f %8.4f |\n"
        "| %8.4f %8.4f %8.4f %8.4f |    | %8.4f %8.4f %8.4f %8.4f |\n"
        "| %8.4f %8.4f %8.4f %8.4f | != | %8.4f %8.4f %8.4f %8.4f |\n"
        "| %8.4f %8.4f %8.4f %8.4f |    | %8.4f %8.4f %8.4f %8.4f |\n",
        a[0][0], a[1][0], a[2][0], a[3][0], b[0][0], b[1][0], b[2][0], b[3][0],
        a[0][1], a[1][1], a[2][1], a[3][1], b[0][1], b[1][1], b[2][1], b[3][1],
        a[0][2], a[1][2], a[2][2], a[3][2], b[0][2], b[1][2], b[2][2], b[3][2],
        a[0][3], a[1][3], a[2][3], a[3][3], b[0][3], b[1][3], b[2][3], b[3][3] 
    );

    test_fatal_raw(t, "matrices not equal", location);
}

#define test_mtx_equal(t, expected, actual) test_mtx_equal_raw(t, expected, actual, __FILE__ ":" STRINGIZE(__LINE__))

void test_matrix_inverse(struct test_context* t) {
    mat4x4 mtx;
    mat4x4 mtx_inverse;
    mat4x4 mtx_inverse_expected;

    matrixFromPosition(mtx, &(struct Vector3){1.0f, 2.0f, 3.0f});
    matrixInv(mtx, mtx_inverse);
    matrixFromPosition(mtx_inverse_expected, &(struct Vector3){-1.0f, -2.0f, -3.0f});
    test_mtx_equal(t, mtx_inverse_expected, mtx_inverse);

    matrixFromScale(mtx, 0.25f);
    matrixInv(mtx, mtx_inverse);
    matrixFromScale(mtx_inverse_expected, 4.0f);
    test_mtx_equal(t, mtx_inverse_expected, mtx_inverse);

    mat4x4 mtx_random = {
        {0, 6, 8, 5},
        {7, 0, 6, 9},
        {9, 6, 2, 3},
        {7, 1, 5, 4},
    };
    matrixInv(mtx_random, mtx_inverse);
    mat4x4 mtx_random_inverse_expected = {
        {-0.06206896552, -0.03448275862, 0.04137931034, 0.124137931},
        {0.06896551724, -0.01724137931, 0.1206896552, -0.1379310345},
        {0.07389162562, -0.1256157635, -0.1206896552, 0.2807881773},
        {-0.0009852216749, 0.2216748768, 0.04827586207, -0.2837438424},
    };
    test_mtx_equal(t, mtx_random_inverse_expected, mtx_inverse);
}