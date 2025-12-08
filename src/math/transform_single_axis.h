#ifndef __MATH_TRANSFORM_SINGLE_AXIS_H__
#define __MATH_TRANSFORM_SINGLE_AXIS_H__

#include "vector3.h"
#include "vector2.h"
#include "matrix.h"

struct TransformSingleAxis {
    struct Vector3 position;
    struct Vector2 rotation;
    float scale;
};

typedef struct TransformSingleAxis transform_sa_t;

void transformSaInitIdentity(struct TransformSingleAxis* transform);
void transformSaInit(struct TransformSingleAxis* transform, struct Vector3* pos, struct Vector2* rot, float scale);
void transformSAToMatrix(struct TransformSingleAxis* transform, mat4x4 matrix);

void transformSaTransformPoint(struct TransformSingleAxis* transform, struct Vector3* input, struct Vector3* output);

#endif