#ifndef __COLLISION_CAST_POINT_H__
#define __COLLISION_CAST_POINT_H__

#include "../math/vector3.h"
#include "surface_type.h"

struct cast_point {
    vector3_t pos;
    vector3_t normal;
    float y;
    surface_type_t surface_type;
};

typedef struct cast_point cast_point_t;

static inline void cast_point_set_pos(cast_point_t* cast_point, vector3_t* pos) {
    cast_point->pos = *pos;
}

#endif