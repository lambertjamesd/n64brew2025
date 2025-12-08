#ifndef __RENDER_DEFS_H__
#define __RENDER_DEFS_H__

#include "../math/vector3.h"

#define MODEL_SCALE     128
#define STATIC_SCALE    64
#define WORLD_SCALE     32

#define MODEL_WORLD_SCALE   ((float)WORLD_SCALE / (float)MODEL_SCALE)
#define STATIC_WORLD_SCALE   ((float)WORLD_SCALE / (float)STATIC_SCALE)

#define DEFAULT_CAMERA_FOV          70.0f
#define DEFAULT_CAMERA_FOV_H        86.067076526f

// = sin(DEFAULT_CAMERA_FOV_H / 3)
#define DEFAULT_CAMERA_SIN_FOV_3    0.480055479f

#define DEFAULT_CAMERA_COS_FOV_2    0.730954366f
#define DEFAULT_CAMERA_SIN_FOV_2    0.682426343f

#define DEFAULT_CAMERA_COS_FOV_6    0.968823547f
#define DEFAULT_CAMERA_SIN_FOV_6    0.247751761f

// = sin((180 - DEFAULT_CAMERA_FOV_H) / 2)
#define DEFAULT_CAMERA_SIN_TRI_CORNER 0.968823547f

void pack_position_vector(struct Vector3* input, short output[3]);

#endif