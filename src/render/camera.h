#ifndef __RENDER_CAMERA_H__
#define __RENDER_CAMERA_H__

#include "../math/transform.h"
#include "../math/plane.h"
#include "../math/matrix.h"
#include "../math/ray.h"
#include "../math/vector2.h"
#include <t3d/t3d.h>

struct ClippingPlanes {
    struct Plane planes[5];
};

struct Camera {
    struct Transform transform;
    float fov;
    float near;
    float far;
};

typedef struct Camera camera_t;

void camera_init(struct Camera* camera, float fov, float near, float far);

void camera_apply(struct Camera* camera, T3DViewport* viewport, struct ClippingPlanes* clipping_planes, mat4x4 view_proj_matrix);

static inline void camera_set_near(camera_t* camera, float near) {
    camera->near = near;
}

void camera_screen_to_ray(camera_t* camera, vector2_t* screen_pos, ray_t* ray);
void camera_screen_from_position(camera_t* camera, vector3_t* pos, vector2_t* screen_pos);

#endif