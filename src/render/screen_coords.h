#ifndef __RENDER_SCREEN_COORDS_H__
#define __RENDER_SCREEN_COORDS_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/quaternion.h"
#include "../math/ray.h"
#include "../math/transform.h"

void screen_coords_to_ray(transform_t* camera_transform, float camera_fov, vector2_t* screen_pos, ray_t* ray);
void screen_coords_from_position(transform_t* camera_transform, float camera_fov, vector3_t* pos, vector2_t* screen_pos);

#endif