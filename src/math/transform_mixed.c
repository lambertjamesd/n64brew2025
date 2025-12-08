#include "transform_mixed.h"

void transform_mixed_init(struct transform_mixed* transform_mixed, struct Transform* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_BASIC;
}

void transform_mixed_init_sa(struct transform_mixed* transform_mixed, struct TransformSingleAxis* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_SINGLE_AXIS;
}

void transform_mixed_init_pos(struct transform_mixed* transform_mixed, struct Vector3* transform) {
    transform_mixed->transform = transform;
    transform_mixed->type = TRANSFORM_TYPE_POSITION;
}

struct Vector3* transform_mixed_get_position(struct transform_mixed* transform) {
    return transform->transform;
}

bool transform_rotate_towards(struct TransformSingleAxis* transform, struct Vector3* forward, float max_radians) {
    struct Vector2 target_dir;
    vector2LookDir(&target_dir, forward);

    struct Vector2 max_rotate;
    vector2ComplexFromAngle(max_radians, &max_rotate);

    return vector2RotateTowards(&transform->rotation, &target_dir, &max_rotate, &transform->rotation);
}