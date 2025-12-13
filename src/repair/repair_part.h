#ifndef __REPAIR_REPAIR_PART_H__
#define __REPAIR_REPAIR_PART_H__

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "../collision/mesh_collider.h"

struct repair_part_definition {
    vector3_t start_position;
    quaternion_t start_rotation;
    
    vector3_t end_position;
    quaternion_t end_rotation;
};

struct repair_part {
    transform_t transform;
    tmesh_t mesh;
    mesh_collider_t collider;
};

typedef struct repair_part repair_part_t;

#endif