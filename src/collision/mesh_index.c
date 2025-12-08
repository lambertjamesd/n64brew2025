#include "mesh_index.h"

#define NEXT_TRIANGLE_INDEX(curr)   ((curr) == 2 ? 0 : (curr) + 1)

bool mesh_triangle_filter_edge_contacts(
    struct mesh_triangle_indices* triangle,
    struct Vector3* vertices,
    struct Vector3* contact_normal
) {
    if (!triangle->enabled_edges) {
        return true;
    }

    for (int i = 0; i < 3; i += 1) {
        if (!HAS_EDGE(triangle, i)) {
            continue;
        }

        int next_i = NEXT_TRIANGLE_INDEX(i);
        int next_next_i = NEXT_TRIANGLE_INDEX(next_i);
        struct Vector3 offset;
        struct Vector3 opposite;

        vector3Sub(&vertices[triangle->indices[next_i]], &vertices[triangle->indices[i]], &offset);
        vector3Sub(&vertices[triangle->indices[next_next_i]], &vertices[triangle->indices[i]], &opposite);

        struct Vector3 perp_offset;

        vector3Project(&offset, &opposite, &perp_offset);

        vector3Sub(&perp_offset, &opposite, &perp_offset);

        if (vector3Dot(&perp_offset, contact_normal) > 0.0001f) {
            return false;
        }
    }

    return true;
}