#include "physics.h"

void applySpringForce(struct Vector3* pos, struct Vector3* vel, struct Vector3* target, float force) {
    struct Vector3 offset;
    vector3Sub(target, pos, &offset);
    vector3AddScaled(vel, &offset, force, vel);
}