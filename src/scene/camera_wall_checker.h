#ifndef __CAMERA_WALL_CHECKER_H__
#define __CAMERA_WALL_CHECKER_H__

#include "../math/vector3.h"
#include "../collision/dynamic_object.h"

struct camera_wall_checker {
    vector3_t position;
    vector3_t cast_from;
    float actual_distance;
    dynamic_object_t collider;
};

typedef struct camera_wall_checker camera_wall_checker_t;

void camera_wall_checker_init(camera_wall_checker_t* checker);
void camera_wall_checker_update(camera_wall_checker_t* checker, vector3_t* look_target, vector3_t* position);
void camera_wall_checker_destroy(camera_wall_checker_t* checker);

#endif