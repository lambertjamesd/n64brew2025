#ifndef __PHYSICS_MOVE_TOWARDS_H__
#define __PHYSICS_MOVE_TOWARDS_H__

#include "../math/vector3.h"
#include <stdbool.h>

struct move_towards_parameters {
    float max_speed;
    float max_accel;
};

bool move_towards(
    struct Vector3* position,
    float* speed,
    struct Vector3* target,
    struct move_towards_parameters* parameters
);

#endif