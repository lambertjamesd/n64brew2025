#include "transform_single_axis.h"

#include "../render/defs.h"

void transformSaInitIdentity(struct TransformSingleAxis* transform) {
    transform->position = gZeroVec;
    transform->rotation = gRight2;
    transform->scale = 1.0f;
}

void transformSaInit(struct TransformSingleAxis* transform, struct Vector3* pos, struct Vector2* rot, float scale) {
    transform->position = *pos;
    transform->rotation = *rot;
    transform->scale = scale;
}

void transformSAToMatrix(struct TransformSingleAxis* transform, mat4x4 matrix) {
    float scale = transform->scale * MODEL_WORLD_SCALE;

    matrix[0][0] = transform->rotation.x * scale;
    matrix[0][1] = 0.0f;
    matrix[0][2] = transform->rotation.y * scale;
    matrix[0][3] = 0.0f;

    matrix[1][0] = 0.0f;
    matrix[1][1] = scale;
    matrix[1][2] = 0.0f;
    matrix[1][3] = 0.0f;

    matrix[2][0] = -transform->rotation.y * scale;
    matrix[2][1] = 0.0f;
    matrix[2][2] = transform->rotation.x * scale;
    matrix[2][3] = 0.0f;

    matrix[3][0] = transform->position.x * WORLD_SCALE;
    matrix[3][1] = transform->position.y * WORLD_SCALE;
    matrix[3][2] = transform->position.z * WORLD_SCALE;
    matrix[3][3] = 1.0f;
}

void transformSaTransformPoint(struct TransformSingleAxis* transform, struct Vector3* input, struct Vector3* output) {
    vector3RotateWith2(input, &transform->rotation, output);
    vector3AddScaled(&transform->position, output, transform->scale, output);
}