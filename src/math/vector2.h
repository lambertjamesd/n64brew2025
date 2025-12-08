#ifndef _MATH_VECTOR2_H
#define _MATH_VECTOR2_H

struct Vector2 {
    float x, y;
};

typedef struct Vector2 vector2_t;

extern struct Vector2 gRight2;
extern struct Vector2 gUp2;
extern struct Vector2 gZeroVec2;
extern struct Vector2 gOneVec2;

struct Vector3;

void vector2ComplexMul(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2ComplexConj(struct Vector2* a, struct Vector2* out);
int vector2RotateTowards(struct Vector2* from, struct Vector2* towards, struct Vector2* max, struct Vector2* out);
void vector2ComplexFromAngle(float radians, struct Vector2* out);
void vector2Rotate90(struct Vector2* input, struct Vector2* out);
float vector2Cross(struct Vector2* a, struct Vector2* b);
float vector2Dot(struct Vector2* a, struct Vector2* b);
float vector2MagSqr(struct Vector2* a);
float vector2DistSqr(struct Vector2* a, struct Vector2* b);
void vector2Add(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Scale(struct Vector2* a, float scale, struct Vector2* out);
void vector2Normalize(struct Vector2* a, struct Vector2* out);
void vector2Sub(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Negate(struct Vector2* a, struct Vector2* out);

void vector2Min(struct Vector2* a, struct Vector2* b, struct Vector2* out);
void vector2Max(struct Vector2* a, struct Vector2* b, struct Vector2* out);

void vector2Lerp(struct Vector2* a, struct Vector2* b, float lerp, struct Vector2* out);

void vector2RandomUnitCircle(struct Vector2* result);

void vector2LookDir(struct Vector2* result, struct Vector3* direction);
void vector2ToLookDir(struct Vector2* input, struct Vector3* out);
void vector3RotatedSpeed(struct Vector2* rotation, struct Vector3* result, float speed);

void vector3RotateWith2(struct Vector3* input, struct Vector2* rotation, struct Vector3* result);
void vector3RotateWith2Inv(struct Vector3* input, struct Vector2* rotation, struct Vector3* result);

#endif