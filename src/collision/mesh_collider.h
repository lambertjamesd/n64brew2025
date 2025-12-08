#ifndef __COLLISION_MESH_COLLIDER_H__
#define __COLLISION_MESH_COLLIDER_H__

#include <stdint.h>
#include <stdbool.h>

#include "../math/vector3.h"
#include "../math/box3d.h"
#include "surface_type.h"
#include "mesh_index.h"
#include "kd_tree.h"

struct mesh_collider {
    kd_tree_t index;
};

struct mesh_triangle {
    struct Vector3* vertices;
    struct mesh_triangle_indices triangle;
};

void mesh_triangle_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);

bool mesh_collider_lookup_triangle_indices(struct mesh_collider* collider, struct Box3D* box, kd_triangle_callback callback, void* data, int collision_layers);
bool mesh_collider_shadow_cast(struct mesh_collider* collider, struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

#endif