#ifndef __EFFECTS_MESH_ANIMATION_H__
#define __EFFECTS_MESH_ANIMATION_H__

#include "../render/animator.h"
#include "../render/tmesh.h"
#include "../render/renderable.h"
#include "../math/transform_single_axis.h"
#include <stdbool.h>

struct mesh_animation {
    struct TransformSingleAxis transform;
    struct renderable renderable;
    struct animator animtor;
};

struct mesh_animation* mesh_animation_new(struct Vector3* position, struct Vector2* rotation, struct tmesh* mesh, struct animation_clip* clip);
bool mesh_animation_update(struct mesh_animation* mesh_animation);
void mesh_animation_free(struct mesh_animation* mesh_animation);

#endif