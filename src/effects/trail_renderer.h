#ifndef __EFFECTS_TRAIL_RENDERER_H__
#define __EFFECTS_TRAIL_RENDERER_H__

#include "../math/vector3.h"

#define MAX_POINT_COUNT     8

struct trail_renderer {
    struct Vector3 path[MAX_POINT_COUNT];
};

#endif