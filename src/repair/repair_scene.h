#ifndef __REPAIR_REPAIR_SCENE_H__
#define __REPAIR_REPAIR_SCENE_H__

#include <t3d/t3d.h>

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "repair_part.h"
#include "../render/frame_alloc.h"

struct repair_scene {
    tmesh_t static_meshes;
    repair_part_t* repair_parts;
    uint16_t repair_part_count;

    transform_t camera_transform;
    float camera_fov;
};

typedef struct repair_scene repair_scene_t;

void repair_scene_render(repair_scene_t* scene, T3DViewport* viewport, struct frame_memory_pool* pool);
repair_scene_t* repair_scene_load(const char* filename);
void repair_scene_destroy(repair_scene_t* scene);

extern repair_scene_t* current_repair_scene;

#endif