#include "screen_coords.h"

#include "defs.h"
#include <math.h>

void screen_coords_to_ray(transform_t* camera_transform, float camera_fov, vector2_t* screen_pos, ray_t* ray) {
    float tan_fov = tanf(camera_fov * 0.5f);
    
    vector3_t local_direction = {
        tan_fov * (2.0f * screen_pos->x - SCREEN_WD) * (1.0f / SCREEN_HT),
        -tan_fov * (2.0f * screen_pos->y - SCREEN_HT) * (1.0f / SCREEN_HT), 
        -1.0f,
    };

    vector3Normalize(&local_direction, &local_direction);

    ray->origin = camera_transform->position;
    quatMultVector(&camera_transform->rotation, &local_direction, &ray->dir);
}

void screen_coords_from_position(transform_t* camera_transform, float camera_fov, vector3_t* pos, vector2_t* screen_pos) {
    vector3_t direction;
    vector3Sub(pos, &camera_transform->position, &direction);

    quaternion_t inv_rot;
    quatConjugate(&camera_transform->rotation, &inv_rot);

    vector3_t local_pos;
    quatMultVector(&inv_rot, &direction, &local_pos);

    if (local_pos.z > -0.0001f) {
        screen_pos->x = -100.0f;
        screen_pos->y = -100.0f;
        return;
    }

    float tan_fov = tanf(camera_fov * 0.5f);
    float local_scale = -(float)SCREEN_HT / (local_pos.z * tan_fov);

    local_pos.x *= local_scale;
    local_pos.y *= -local_scale;

    screen_pos->x = (local_pos.x + SCREEN_WD) * 0.5f;
    screen_pos->y = (local_pos.y + SCREEN_HT) * 0.5f;
}