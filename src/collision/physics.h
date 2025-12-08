#ifndef __COLLISION_PHYSICS_H__
#define __COLLISION_PHYSICS_H__

#include "../math/vector3.h"

void applySpringForce(struct Vector3* pos, struct Vector3* vel, struct Vector3* target, float force);

#endif