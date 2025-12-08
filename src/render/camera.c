#include "camera.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include <math.h>

#include "../math/matrix.h"
#include "defs.h"

void camera_init(struct Camera* camera, float fov, float near, float far) {
    transformInitIdentity(&camera->transform);
    camera->fov = fov;
    camera->near = near;
    camera->far = far;
}


void camera_extract_clipping_plane(float viewPersp[4][4], struct Plane* output, int axis, float direction) {
    output->normal.x = viewPersp[0][axis] * direction + viewPersp[0][3];
    output->normal.y = viewPersp[1][axis] * direction + viewPersp[1][3];
    output->normal.z = viewPersp[2][axis] * direction + viewPersp[2][3];
    output->d = viewPersp[3][axis] * direction + viewPersp[3][3];

    float mult = 1.0f / sqrtf(vector3MagSqrd(&output->normal));
    vector3Scale(&output->normal, &output->normal, mult);
    output->d *= mult;
}

void camera_apply(struct Camera* camera, T3DViewport* viewport, struct ClippingPlanes* clipping_planes, mat4x4 view_proj_matrix) {
    float tan_fov = tanf(camera->fov * (0.5f * 3.14159f / 180.0f));
    float aspect_ratio = (float)viewport->size[0] / (float)viewport->size[1];

    float near = camera->near * WORLD_SCALE;
    float far = camera->far * WORLD_SCALE;

    matrixPerspective(
        viewport->matProj.m, 
        -aspect_ratio * tan_fov * near,
        aspect_ratio * tan_fov * near,
        tan_fov * near,
        -tan_fov * near,
        near,
        far
    );
    t3d_viewport_set_w_normalize(viewport, near, far);

    struct Transform inverse;
    transformInvert(&camera->transform, &inverse);
    inverse.position = gZeroVec;
    transformToMatrix(&inverse, viewport->matCamera.m);
    viewport->_isCamProjDirty = true;
    
    if (!view_proj_matrix) {
        return;
    }

    matrixMul(viewport->matProj.m, viewport->matCamera.m, view_proj_matrix);

    if (!clipping_planes) {
        return;
    }

    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[0], 0, 1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[1], 0, -1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[2], 1, 1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[3], 1, -1.0f);
    camera_extract_clipping_plane(view_proj_matrix, &clipping_planes->planes[4], 2, 1.0f);
}