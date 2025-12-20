#ifndef __COLLISION_RAYCAST_H__
#define __COLLISION_RAYCAST_H__

#include <stdbool.h>
#include "../math/ray.h"

bool triangle_raycast(struct Ray* ray, vector3_t* vertices, uint16_t* indices, float* distance);

#endif