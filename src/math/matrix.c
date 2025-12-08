#include "matrix.h"

void matrixPerspective(float matrix[4][4], float l, float r, float t, float b, float near, float far) {
    matrix[0][0] = 2.0f * near / (r - l);
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;

    matrix[1][0] = 0.0f;
    matrix[1][1] = 2.0f * near / (t - b);
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;

    matrix[2][0] = (r + l) / (r - l);
    matrix[2][1] = (t + b) / (t - b);
    matrix[2][2] = -(far + near) / (far - near);
    matrix[2][3] = -1;

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = -2.0f * far * near / (far - near);
    matrix[3][3] = 0.0f;
}

float matrixNormalizedZValue(float depth, float near, float far) {
    if (depth >= -near) {
        return -1.0f;
    }

    if (depth <= -far) {
        return 1.0f;
    }

    return (far * (depth + near) + 2.0 * far * near) / (depth * (far - near));
}

void matrixVec3Mul(float matrix[4][4], struct Vector3* input, struct Vector4* output) {
    output->x = matrix[0][0] * input->x + matrix[1][0] * input->y + matrix[2][0] * input->z + matrix[3][0];
    output->y = matrix[0][1] * input->x + matrix[1][1] * input->y + matrix[2][1] * input->z + matrix[3][1];
    output->z = matrix[0][2] * input->x + matrix[1][2] * input->y + matrix[2][2] * input->z + matrix[3][2];
    output->w = matrix[0][3] * input->x + matrix[1][3] * input->y + matrix[2][3] * input->z + matrix[3][3];
}

void matrixFromBasis(float matrix[4][4], struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z) {
    matrix[0][0] = x->x;
    matrix[0][1] = x->y;
    matrix[0][2] = x->z;
    matrix[0][3] = 0.0f;

    matrix[1][0] = y->x;
    matrix[1][1] = y->y;
    matrix[1][2] = y->z;
    matrix[1][3] = 0.0f;

    matrix[2][0] = z->x;
    matrix[2][1] = z->y;
    matrix[2][2] = z->z;
    matrix[2][3] = 0.0f;

    matrix[3][0] = origin->x;
    matrix[3][1] = origin->y;
    matrix[3][2] = origin->z;
    matrix[3][3] = 1.0f;
}

void matrixFromPosition(float matrix[4][4], struct Vector3* position) {
    matrix[0][0] = 1.0f;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;

    matrix[1][0] = 0.0f;
    matrix[1][1] = 1.0f;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;

    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = 1.0f;
    matrix[2][3] = 0.0f;

    matrix[3][0] = position->x;
    matrix[3][1] = position->y;
    matrix[3][2] = position->z;
    matrix[3][3] = 1.0f;
}

void matrixFromScale(float matrix[4][4], float scale) {
    matrix[0][0] = scale;
    matrix[0][1] = 0.0f;
    matrix[0][2] = 0.0f;
    matrix[0][3] = 0.0f;

    matrix[1][0] = 0.0f;
    matrix[1][1] = scale;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;

    matrix[2][0] = 0.0f;
    matrix[2][1] = 0.0f;
    matrix[2][2] = scale;
    matrix[2][3] = 0.0f;

    matrix[3][0] = 0.0f;
    matrix[3][1] = 0.0f;
    matrix[3][2] = 0.0f;
    matrix[3][3] = 1.0f;
}

void matrixApplyScaledPos(float matrix[4][4], struct Vector3* position, float scale) {
    matrix[3][0] = position->x * scale;
    matrix[3][1] = position->y * scale;
    matrix[3][2] = position->z * scale;
}

void matrixApplyScale(float matrix[4][4], float scale) {
    for (int row = 0; row < 3; row += 1) {
        for (int col = 0; col < 3; col += 1) {
            matrix[row][col] *= scale;
        }
    }
}

void matrixMul(float a[4][4], float b[4][4], float output[4][4]) {
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) {
            output[x][y] = 0.0f;
            for (int j = 0; j < 4; ++j) {
                output[x][y] += a[j][y] * b[x][j];
            }
        }
    }
}

bool matrixInv(float input[4][4], float output[4][4]) {    
    for (int y = 0; y < 4; y += 1) {
        for (int x = 0; x < 4; x += 1) {
            float element = 0.0f;

            int x0 = (x + 1) & 0x3;
            int x1 = (x + 2) & 0x3;
            int x2 = (x + 3) & 0x3;
            int y0 = (y + 1) & 0x3;
            int y1 = (y + 2) & 0x3;
            int y2 = (y + 3) & 0x3;

            for (int i = 0; i < 3; i += 1) {
                element += input[y0][x0] * input[y1][x1] * input[y2][x2] -
                    input[y2][x0] * input[y1][x1] * input[y0][x2];

                int tmp = x0;
                x0 = x2;
                x2 = x1;
                x1 = tmp;
            }

            // transpose matrix and negate every other cell
            output[x][y] = ((x ^ y) & 0x1) ? -element : element;
        }
    }

    float det = input[0][0] * output[0][0] + input[0][1] * output[1][0] + input[0][2] * output[2][0] + input[0][3] * output[3][0];

    if (det == 0.0f)
        return false;

    det = 1.0 / det;

    for (int y = 0; y < 4; y += 1) {
        for (int x = 0; x < 4; x += 1) {
            output[y][x] = output[y][x] * det;
        }
    }

    return true;
}