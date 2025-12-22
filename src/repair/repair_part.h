#ifndef __REPAIR_REPAIR_PART_H__
#define __REPAIR_REPAIR_PART_H__

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "../collision/mesh_collider.h"
#include "../render/frame_alloc.h"
#include "../math/ray.h"
#include "../scene/scene_definition.h"
#include <stdio.h>

struct repair_collider {
    vector3_t* vertices;
    uint16_t* indices;
    uint16_t triangle_count;
};

typedef struct repair_collider repair_collider_t;

struct repair_part {
    transform_t transform;
    tmesh_t mesh;
    repair_collider_t collider;
    quaternion_t target_rotation;

    vector3_t end_position;
    quaternion_t end_rotation;

    boolean_variable has_part;

    bool is_connected: 1;
    bool is_present: 1;
};

typedef struct repair_part repair_part_t;

void repair_part_load(repair_part_t* part, FILE* file);
void repair_part_destroy(repair_part_t* part);

void repair_part_render(repair_part_t* part, struct frame_memory_pool* pool);
bool repair_part_raycast(repair_part_t* part, ray_t* ray, float* distance);
void repair_part_update(repair_part_t* part);
void repair_part_set_complete(repair_part_t* part);

#endif