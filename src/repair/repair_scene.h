#ifndef __REPAIR_REPAIR_SCENE_H__
#define __REPAIR_REPAIR_SCENE_H__

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "repair_part.h"

struct repair_scene {
    tmesh_t* static_meshes;
    repair_part_t* repair_parts;
    uint16_t static_mesh_count;
    uint16_t repair_part_count;

    transform_t camera_position;
};

typedef struct repair_scene repair_scene_t;

#endif