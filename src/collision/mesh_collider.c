#include "mesh_collider.h"

#include <math.h>
#include <stdio.h>
#include "../math/minmax.h"

#include <libdragon.h>

#define MAX_INDEX_SET_SIZE 64

void mesh_triangle_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output) {
    struct mesh_triangle* triangle = (struct mesh_triangle*)data;

    int idx = 0;
    float distance = vector3Dot(&triangle->vertices[triangle->triangle.indices[0]], direction);

    float check = vector3Dot(&triangle->vertices[triangle->triangle.indices[1]], direction);

    if (check > distance) {
        idx = 1;
        distance = check;
    }

    check = vector3Dot(&triangle->vertices[triangle->triangle.indices[2]], direction);

    if (check > distance) {
        idx = 2;
    }

    *output = triangle->vertices[triangle->triangle.indices[idx]];
}

bool mesh_collider_lookup_triangle_indices(struct mesh_collider* collider, struct Box3D* box, kd_triangle_callback callback, void* data, int collision_layers) {
    return kd_tree_lookup(&collider->index, box, data, callback, collision_layers);
}

bool mesh_triangle_shadow_cast(struct mesh_triangle_indices indices, struct Vector3* vertices, struct Vector3* starting_point, struct mesh_shadow_cast_result* result) {
    struct Vector3 triangle_edges[3];

    struct Vector3 cast_offset;

    for (int i = 0; i < 3; ++i) {
        int next_index = i == 2 ? 0 : i + 1;
        vector3Sub(&vertices[indices.indices[next_index]], &vertices[indices.indices[i]], &triangle_edges[i]);
        vector3Sub(starting_point, &vertices[indices.indices[i]], &cast_offset);

        if (cast_offset.z * triangle_edges[i].x - cast_offset.x * triangle_edges[i].z > 0.00001f) {
            return false;
        }
    }

    struct Vector3 normal;
    vector3Cross(&triangle_edges[0], &triangle_edges[1], &normal);

    if (fabsf(normal.y) < 0.00001f) {
        return false;
    }

    float y = vertices[indices.indices[2]].y - (normal.x * cast_offset.x + normal.z * cast_offset.z) / normal.y;

    if (indices.surface_type != SURFACE_TYPE_WATER && y > starting_point->y + 0.0001f) {
        return false;
    }

    result->y = y;
    result->normal = normal;
    result->surface_type = indices.surface_type;

    return true;
}

bool mesh_collider_shadow_cast(struct mesh_collider* collider, struct Vector3* starting_point, struct mesh_shadow_cast_result* result) {
    return kd_tree_shadow_cast(&collider->index, starting_point, result);
}