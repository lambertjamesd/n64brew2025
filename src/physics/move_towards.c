#include "move_towards.h"

#include "../time/time.h"
#include "../math/mathf.h"
#include <math.h>
#include <stdio.h>

// dP = v*t + 0.5*a*t*t
// t = v/a
// stop_distance = 0.5*v*v/a

// distance * a * 2.0 < v*v

// sqrtf(stop_distance * a * 2)

bool move_towards(
    struct Vector3* position,
    float* speed,
    struct Vector3* target,
    struct move_towards_parameters* parameters
) {
    float distance = sqrtf(vector3DistSqrd(position, target));

    float target_speed = parameters->max_speed;

    float stop_distance_check = 2.0f * distance * parameters->max_accel;

    float speed_value = *speed;

    if (stop_distance_check < speed_value * speed_value) {
        target_speed = 0.0f;
    }

    speed_value = mathfMoveTowards(speed_value, target_speed, parameters->max_accel * fixed_time_step);

    if (fabsf(speed_value) > 0.001f) {
        vector3MoveTowards(position, target, speed_value * fixed_time_step, position);
    }

    *speed = speed_value;

    return distance < 0.0001f;
}