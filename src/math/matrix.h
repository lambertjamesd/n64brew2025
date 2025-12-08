#ifndef __MATRIX_H__
#define __MATRIX_H__

#include <stdbool.h>
#include "vector4.h"
#include "vector3.h"

typedef float mat4x4[4][4];

void matrixPerspective(float matrix[4][4], float l, float r, float top, float b, float near, float far);

float matrixNormalizedZValue(float depth, float nearPlane, float farPlane);

void matrixVec3Mul(float matrix[4][4], struct Vector3* input, struct Vector4* output);

void matrixMul(float a[4][4], float b[4][4], float output[4][4]);

void matrixFromBasis(float matrix[4][4], struct Vector3* origin, struct Vector3* x, struct Vector3* y, struct Vector3* z);

void matrixFromPosition(float matrix[4][4], struct Vector3* position);

void matrixFromScale(float matrix[4][4], float scale);

void matrixApplyScaledPos(float matrix[4][4], struct Vector3* position, float scale);
void matrixApplyScale(float matrix[4][4], float scale);

bool matrixInv(float input[4][4], float output[4][4]);

#endif