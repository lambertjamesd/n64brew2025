#ifndef _OVERWORLD_OVERWORLD_
#define _OVERWORLD_OVERWORLD_

#include "../collision/mesh_collider.h"
#include "../cutscene/expression.h"
#include "../math/vector2.h"
#include "../render/tmesh.h"
#include "../util/hash_map.h"
#include "../entity/entity_id.h"
#include "../particles/static_particles.h"
#include "../scene/scene_definition.h"
#include <stdint.h>
#include <stdio.h>
#include <t3d/t3d.h>

struct overworld_tile_def {
    uint32_t visual_block_offset;
    uint32_t actor_block_offset;
};

struct overworld_tile {
    struct tmesh* terrain_meshes;
    rspq_block_t** render_blocks;
    float starting_y;
    struct tmesh** detail_meshes;
    uint16_t terrain_mesh_count;
    uint16_t detail_mesh_count;
    uint16_t detail_count;
    uint16_t static_particle_count;
    T3DMat4FP* detail_matrices;
    static_particles_t* static_particles;
};

struct overworld_actor_spawn_information {
    uint16_t entity_type_id;
    integer_variable scene_variable;
    struct expression expression;
    void* entity_def;
};

struct overworld_actor_spawn_location {
    uint8_t x, y;
    uint16_t spawn_id_offset;
};

struct overworld_actor {
    struct overworld_actor* next;
    entity_id entity_id;
    uint8_t tile_x, tile_y;
    uint8_t x, y;
    integer_variable scene_variable;
    uint32_t spawn_id;
};

struct overworld_actor_tile {
    struct mesh_collider collider;
    uint16_t x;
    uint16_t y;

    uint16_t active_spawn_locations;
    uint16_t total_spawn_locations;
    
    uint32_t first_spawn_id;

    struct overworld_actor_spawn_location* spawn_locations;
    struct overworld_actor_spawn_information* spawn_information;
    char* string_table;
};

struct overworld_tile_render_block {
    rspq_block_t** render_blocks;
    struct overworld_tile* tile;
    float starting_y;
    uint8_t x;
    uint8_t z;
    uint8_t y_height;
};

#define LOD0_SORT_DIRECTION_COUNT   4

struct overworld_lod0_entry {
    struct tmesh meshes[LOD0_SORT_DIRECTION_COUNT];
    int16_t x, z;
    uint16_t priority;
};

struct overworld_lod0 {
    struct overworld_lod0_entry* entries;
    uint8_t entry_count;
};

#define NO_TILE_COORD   0xFFFF
#define MAX_ACTIVE_ACTORS   128
#define LOADED_TILE_ARRAY_SIZE  2

struct overworld {
    uint16_t tile_x, tile_y;
    struct Vector2 min;
    float inv_tile_size;
    float tile_size;
    struct overworld_lod0 lod0;
    struct overworld_tile_def* tile_definitions;
    FILE* file;

    // tile locations modulo wrap to share slots
    struct overworld_tile* loaded_tiles[4][4];
    struct overworld_tile_render_block render_blocks[4][4];

    struct overworld_actor_tile* loaded_actor_tiles[LOADED_TILE_ARRAY_SIZE][LOADED_TILE_ARRAY_SIZE];

    struct { uint16_t x; uint16_t y; } load_next;

    struct hash_map loaded_actors;

    struct overworld_actor actors[MAX_ACTIVE_ACTORS];
    struct overworld_actor* next_free_actor;
    struct overworld_actor* next_active_actor;
};

typedef struct overworld overworld_t;

struct overworld* overworld_load(const char* filename);
void overworld_free(struct overworld* overworld);

#endif