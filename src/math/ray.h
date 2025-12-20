#ifndef __RAY_H__
#define __RAY_H__

#include "vector3.h"
#include "transform.h"

struct Ray {
    struct Vector3 origin;
    struct Vector3 dir;
};

typedef struct Ray ray_t;

void rayTransform(struct Transform* transform, struct Ray* ray, struct Ray* output);
float rayDetermineDistance(struct Ray* ray, struct Vector3* point);

#endif