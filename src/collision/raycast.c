#include "./raycast.h"

bool triangle_raycast(struct Ray* ray, vector3_t* vertices, uint16_t* indices, float* distance) {
    float start_distance = *distance;
    for (int i = 0; i < 3; i += 1) {
        int next_i = i == 2 ? 0 : i + 1;

        vector3_t* a = &vertices[indices[i]];
        vector3_t* b = &vertices[indices[next_i]];
        vector3_t offset;
        vector3Sub(a, &ray->origin, &offset);

        vector3_t edge;
        vector3Sub(b, a, &edge);
        vector3_t side;
        vector3Cross(&edge, &offset, &side);

        if (vector3Dot(&side, &ray->dir) > 0.000001f) {
            return false;
        }

        float check = vector3Dot(&offset, &ray->dir);

        if (check < start_distance) {
            start_distance = check;
        }
    }

    bool result = start_distance != *distance;
    *distance = start_distance;
    return result;
}