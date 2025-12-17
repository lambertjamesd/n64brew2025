#ifndef __REPAIR_REPAIR_PART_H__
#define __REPAIR_REPAIR_PART_H__

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "../collision/mesh_collider.h"
#include <stdio.h>

struct repair_part {
    transform_t transform;
    tmesh_t mesh;
    mesh_collider_t collider;

    vector3_t end_position;
    quaternion_t end_rotation;
};

typedef struct repair_part repair_part_t;

void repair_part_load(repair_part_t* part, FILE* file);
void repair_part_destroy(repair_part_t* part);

#endif