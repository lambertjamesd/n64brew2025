#ifndef __REPAIR_REPAIR_SCENE_H__
#define __REPAIR_REPAIR_SCENE_H__

#include <t3d/t3d.h>

#include "../render/tmesh.h"
#include "../math/transform.h"
#include "repair_part.h"
#include "../render/frame_alloc.h"
#include "../math/vector2.h"
#include "../render/material.h"

struct repar_scene_assets {
    material_t cursor_material;
};

typedef struct repar_scene_assets repar_scene_assets_t;

struct repair_scene {
    tmesh_t static_meshes;
    repair_part_t* repair_parts;
    uint16_t repair_part_count;

    transform_t camera_transform;
    float camera_fov;

    vector2_t screen_cursor;
    repair_part_t* grabbed_part;
    repair_part_t* hovered_part;

    repar_scene_assets_t assets;

    boolean_variable puzzle_complete;
    scene_entry_point exit_scene;

    bool is_missing_parts: 1;
    bool is_complete: 1;
};

typedef struct repair_scene repair_scene_t;

void repair_scene_render(repair_scene_t* scene, T3DViewport* viewport, struct frame_memory_pool* pool);
repair_scene_t* repair_scene_load(const char* filename);
void repair_scene_destroy(repair_scene_t* scene);

extern repair_scene_t* current_repair_scene;

#endif