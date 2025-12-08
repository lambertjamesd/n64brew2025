#ifndef __COLLISION_MESH_INDEX_H__
#define __COLLISION_MESH_INDEX_H__

#include <stdint.h>
#include <stdbool.h>
#include "../math/vector3.h"
#include "surface_type.h"

struct mesh_triangle_indices {
    uint16_t indices[3];
    uint8_t surface_type;
    uint8_t enabled_edges;
};

#define HAS_EDGE(triangle, edge_index)  (((triangle)->enabled_edges & (1 << (edge_index))) != 0)

struct mesh_shadow_cast_result {
    float y;
    struct Vector3 normal;
    enum surface_type surface_type;
};

typedef struct mesh_triangle_indices mesh_triangle_indices_t;

bool mesh_triangle_filter_edge_contacts(
    struct mesh_triangle_indices* triangle,
    struct Vector3* vertices,
    struct Vector3* contact_normal
);

#endif