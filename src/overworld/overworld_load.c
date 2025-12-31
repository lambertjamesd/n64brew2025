#include "overworld_load.h"

#include "../collision/collision_scene.h"
#include "../cutscene/evaluation_context.h"
#include "../cutscene/expression_evaluate.h"
#include "../math/transform.h"
#include "../objects/empty.h"
#include "../render/render_scene.h"
#include "../resource/mesh_collider.h"
#include "../resource/tmesh_cache.h"
#include "../scene/scene_loader.h"
#include "../scene/scene.h"
#include "../entity/entity_spawner.h"
#include "overworld_render.h"
#include <assert.h>
#include <malloc.h>
#include <memory.h>

struct overworld_tile* overworld_tile_load(FILE* file) {
    uint8_t mesh_count;
    fread(&mesh_count, 1, 1, file);
    uint16_t detail_mesh_count;
    fread(&detail_mesh_count, 2, 1, file);
    uint16_t detail_count;
    fread(&detail_count, 2, 1, file);

    struct overworld_tile* result = malloc(
        sizeof(struct overworld_tile) +
        sizeof(struct tmesh) * mesh_count +
        sizeof(rspq_block_t*) * mesh_count +
        sizeof(struct tmesh*) * detail_mesh_count
    );

    fread(&result->starting_y, sizeof(float), 1, file);
    result->terrain_mesh_count = mesh_count;
    result->detail_mesh_count = detail_mesh_count;
    result->detail_count = detail_count;

    result->terrain_meshes = (struct tmesh*)(result + 1);
    result->render_blocks = (rspq_block_t**)(result->terrain_meshes + mesh_count);
    result->detail_meshes = (struct tmesh**)(result->render_blocks + mesh_count);
    result->detail_matrices = detail_count ? malloc(sizeof(T3DMat4FP) * detail_count) : NULL;

    for (int i = 0; i < detail_mesh_count; i += 1) {
        uint8_t filename_len;
        fread(&filename_len, 1, 1, file);
        char filename[filename_len + 1];
        fread(filename, filename_len, 1, file);
        filename[filename_len] = '\0';

        result->detail_meshes[i] = tmesh_cache_load(filename);
    }

    T3DMat4FP* curr_matrix = UncachedAddr(result->detail_matrices);

    for (int i = 0; i < mesh_count; i += 1) {
        
        tmesh_load(&result->terrain_meshes[i], file);
        rspq_block_begin();
        rspq_block_run(result->terrain_meshes[i].material->block);
        rspq_block_run(result->terrain_meshes[i].block);
        
        uint16_t block_detail_count;
        fread(&block_detail_count, 2, 1, file);
        
        struct material* curr_material = NULL;
        for (int detail_index = 0; detail_index < block_detail_count; detail_index += 1) {
            uint16_t detail_type;
            fread(&detail_type, 2, 1, file);
            struct Transform transform;
            fread(&transform, sizeof(struct Transform), 1, file);

            struct tmesh* mesh = result->detail_meshes[detail_type];

            if (curr_material != mesh->material) {
                rspq_block_run(mesh->material->block);
                curr_material = mesh->material;
            }

            T3DMat4 mtx;
            transformToMatrix(&transform, mtx.m);
            t3d_mat4_to_fixed(curr_matrix, &mtx);
            t3d_matrix_push(curr_matrix);
            rspq_block_run(mesh->block);
            t3d_matrix_pop(1);

            ++curr_matrix;
        }

        result->render_blocks[i] = rspq_block_end();
    }

    result->static_particles = static_particles_load(&result->static_particle_count, file);

    return result;
}

void overworld_tile_free(struct overworld_tile* tile) {
    for (int i = 0; i < tile->terrain_mesh_count; i += 1) {
        tmesh_release(&tile->terrain_meshes[i]);
    }
    for (int i = 0; i < tile->detail_mesh_count; i += 1) {
        tmesh_cache_release(tile->detail_meshes[i]);
    }
    static_particles_release(tile->static_particles, tile->static_particle_count);
    free(tile->detail_matrices);
    free(tile);
}

void overworld_actor_tile_load_entities(struct overworld_actor_tile* tile, FILE* file) {
    uint16_t entity_count;
    fread(&entity_count, sizeof(uint16_t), 1, file);
    tile->active_spawn_locations = entity_count;
    tile->total_spawn_locations = entity_count;

    if (!entity_count) {
        tile->first_spawn_id = 0;
        tile->spawn_locations = NULL;
        tile->spawn_information = NULL;
        tile->string_table = NULL;
        return;
    }

    uint32_t entity_definition_size;
    uint16_t expression_size;
    uint16_t string_sizes;

    fread(&entity_definition_size, sizeof(uint32_t), 1, file);
    fread(&expression_size, sizeof(uint16_t), 1, file);
    fread(&string_sizes, sizeof(uint16_t), 1, file);

    void* entity_definition_data = malloc(entity_definition_size);
    void* expression_data = malloc(expression_size);
    void* string_table = string_sizes ? malloc(string_sizes) : NULL;

    fread(entity_definition_data, entity_definition_size, 1, file);
    fread(expression_data, expression_size, 1, file);
    if (string_sizes) {
        fread(string_table, string_sizes, 1, file);
    }
    tile->string_table = string_table;

    fread(&tile->first_spawn_id, sizeof(uint32_t), 1, file);

    tile->spawn_locations = malloc(sizeof(struct overworld_actor_spawn_location) * entity_count);
    fread(tile->spawn_locations, sizeof(struct overworld_actor_spawn_location), entity_count, file);

    tile->spawn_information = malloc(sizeof(struct overworld_actor_spawn_information) * entity_count);
    struct overworld_actor_spawn_information* end = tile->spawn_information + entity_count;

    for (struct overworld_actor_spawn_information* curr = tile->spawn_information; curr < end; curr += 1) {
        uint16_t entity_type_id;
        fread(&entity_type_id, sizeof(uint16_t), 1, file);
        fread(&curr->scene_variable, sizeof(integer_variable), 1, file);

        struct entity_definition* def = entity_def_get(entity_type_id);
        assert(def);
        curr->entity_type_id = entity_type_id;

        scene_entity_apply_types(entity_definition_data, string_table, def->fields, def->field_count);
        curr->entity_def = entity_definition_data;
        
        struct expression_header* header = expression_data;
        assert(header->header == EXPECTED_EXPR_HEADER);
        curr->expression.expression_program = header + 1;

        entity_definition_data = (char*)entity_definition_data + def->definition_size;
        expression_data = (char*)curr->expression.expression_program + header->len;
    }
}

void overworld_actor_tile_mark_spawned(struct overworld* overworld, struct overworld_actor_tile* tile) {
    struct overworld_actor_spawn_location* curr = tile->spawn_locations;
    struct overworld_actor_spawn_location* end = curr + tile->active_spawn_locations;
    struct overworld_actor_spawn_location* write = curr;

    while (curr < end) {
        if (hash_map_get(&overworld->loaded_actors, tile->first_spawn_id + (int)curr->spawn_id_offset)) {
            ++curr;
            continue;
        }

        if (write != curr) {
            *write = *curr;
        }

        ++curr;
        ++write;
    }

    tile->active_spawn_locations = write - tile->spawn_locations;
}

struct overworld_actor_tile* overworld_actor_tile_load(FILE* file) {
    struct overworld_actor_tile* result = malloc(sizeof(struct overworld_actor_tile));
    mesh_collider_load(&result->collider, file);
    overworld_actor_tile_load_entities(result, file);
    return result;
}

void overworld_actor_tile_free(struct overworld_actor_tile* tile) {
    mesh_collider_release(&tile->collider);

    if (tile->spawn_information) {
        free(tile->spawn_information[0].entity_def);
        free((char*)tile->spawn_information[0].expression.expression_program - sizeof(struct expression_header));
        free(tile->spawn_information);
        free(tile->spawn_locations);
        free(tile->string_table);
    }

    free(tile);
}

// EXPR
#define EXPECTED_OVERWORLD_HEADER 0x4F565744

void overworld_render_step(void* data, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    struct overworld* overworld = (struct overworld*)data;
    overworld_render(overworld, view_proj_matrix, camera, viewport, pool);
}

struct overworld* overworld_load(const char* filename) {
    struct overworld* result = malloc(sizeof(struct overworld));
    memset(result, 0, sizeof(struct overworld));

    result->file = fopen(filename, "rb");
    assert(result->file);

    int header;
    fread(&header, 1, 4, result->file);
    assert(header == EXPECTED_OVERWORLD_HEADER);

    fread(&result->tile_x, sizeof(uint16_t), 1, result->file);
    fread(&result->tile_y, sizeof(uint16_t), 1, result->file);
    fread(&result->min, sizeof(struct Vector2), 1, result->file);
    fread(&result->tile_size, sizeof(float), 1, result->file);

    fread(&result->lod0.entry_count, 1, 1, result->file);

    result->lod0.entries = malloc(sizeof(struct overworld_lod0_entry) * result->lod0.entry_count);

    for (int i = 0; i < result->lod0.entry_count; i += 1) {
        fread(&result->lod0.entries[i].x, sizeof(uint16_t), 1, result->file);
        fread(&result->lod0.entries[i].z, sizeof(uint16_t), 1, result->file);
        fread(&result->lod0.entries[i].priority, sizeof(uint16_t), 1, result->file);
        for (int direction_index = 0; direction_index < LOD0_SORT_DIRECTION_COUNT; direction_index += 1) {
            tmesh_load(&result->lod0.entries[i].meshes[direction_index], result->file);
        }
    }

    result->inv_tile_size = 1.0f / result->tile_size;
    result->tile_definitions = malloc(result->tile_x * result->tile_y * sizeof(struct overworld_tile_def));

    fread(result->tile_definitions, sizeof(struct overworld_tile_def), result->tile_x * result->tile_y, result->file);
    
    result->load_next.x = NO_TILE_COORD;
    result->load_next.y = NO_TILE_COORD;

    render_scene_add_step(overworld_render_step, result);

    hash_map_init(&result->loaded_actors, MAX_ACTIVE_ACTORS);

    for (int i = 0; i < MAX_ACTIVE_ACTORS; i += 1) {
        result->actors[i].next = i + 1 < MAX_ACTIVE_ACTORS ? &result->actors[i + 1] : NULL;
    }

    result->next_free_actor = &result->actors[0];
    result->next_active_actor = NULL;

    return result;
}

void overworld_free(struct overworld* overworld) {
    for (int x = 0; x < 4; x += 1) {
        for (int y = 0; y < 4; y += 1) {
            if (overworld->loaded_tiles[x][y]) {
                overworld_tile_free(overworld->loaded_tiles[x][y]);
            }
        }
    }

    for (
        struct hash_map_entry* entry = hash_map_next(&overworld->loaded_actors, NULL);
        entry;
        entry = hash_map_next(&overworld->loaded_actors, NULL)
    ) {
        struct overworld_actor* actor = (struct overworld_actor*)entry->value;
        entity_despawn(actor->entity_id);
        expression_set_integer(actor->scene_variable, 0);
    }

    hash_map_destroy(&overworld->loaded_actors);

    for (int i = 0; i < overworld->lod0.entry_count; i += 1) {
        for (int direction_index = 0; direction_index < LOD0_SORT_DIRECTION_COUNT; direction_index += 1) {
            tmesh_release(&overworld->lod0.entries[i].meshes[direction_index]);
        }
    }
    free(overworld->lod0.entries);

    render_scene_remove_step(overworld);

    fclose(overworld->file);
    free(overworld->tile_definitions);
    free(overworld);
}

void overworld_check_loaded_tiles(struct overworld* overworld) {
    if (overworld->load_next.x == NO_TILE_COORD || overworld->load_next.y == NO_TILE_COORD) {
        return;
    }

    int slot_x = overworld->load_next.x & 0x3;
    int slot_y = overworld->load_next.y & 0x3;

    if (overworld->loaded_tiles[slot_x][slot_y]) {
        rdpq_call_deferred((void (*)(void*))overworld_tile_free, overworld->loaded_tiles[slot_x][slot_y]);
    }

    fseek(overworld->file, overworld->tile_definitions[overworld->load_next.x + overworld->load_next.y * overworld->tile_x].visual_block_offset, SEEK_SET);
    
    struct overworld_tile* tile = overworld_tile_load(overworld->file);
    overworld->loaded_tiles[slot_x][slot_y] = tile;
    overworld->render_blocks[slot_x][slot_y] = (struct overworld_tile_render_block){
        .render_blocks = tile->render_blocks,
        .tile = tile,
        .x = overworld->load_next.x,
        .z = overworld->load_next.y,
        .y_height = tile->terrain_mesh_count,
        .starting_y = tile->starting_y,
    };

    overworld->load_next.x = NO_TILE_COORD;
    overworld->load_next.y = NO_TILE_COORD;
}

#define COLLIDER_SPAWN_RADIUS   100.0f
#define DESPAWN_RADIUS          90.0f
#define SPAWN_IN_RADIUS         80.0f

struct overworld_actor* overworld_malloc_actor(struct overworld* overworld) {
    struct overworld_actor* result = overworld->next_free_actor;

    if (!result) {
        return NULL;
    }

    overworld->next_free_actor = result->next;
    result->next = NULL;

    return result;
}

struct overworld_actor* overworld_actor_spawn(struct overworld* overworld, struct overworld_actor_tile* tile, struct overworld_actor_spawn_location* spawn_location) {
    struct overworld_actor_spawn_information* info = &tile->spawn_information[spawn_location->spawn_id_offset];

    struct evaluation_context context;
    evaluation_context_init(&context, 0);
    expression_evaluate(&context, &info->expression);
    int should_spawn = evaluation_context_pop(&context);
    evaluation_context_destroy(&context);

    if (!should_spawn) {
        return NULL;
    }

    struct entity_definition* entity_def = entity_def_get(info->entity_type_id);

    if (!entity_def) {
        return NULL;
    }

    struct overworld_actor* result = overworld_malloc_actor(overworld);

    if (!result) {
        return NULL;
    }

    int spawn_id =(int)tile->first_spawn_id + (int)spawn_location->spawn_id_offset;

    result->spawn_id = spawn_id;
    result->entity_id = entity_spawn(info->entity_type_id, info->entity_def);
    result->tile_x = tile->x;
    result->tile_y = tile->y;
    result->x = spawn_location->x;
    result->y = spawn_location->y;
    result->next = overworld->next_active_actor;
    result->scene_variable = info->scene_variable;
    overworld->next_active_actor = result;
    hash_map_set(&overworld->loaded_actors, spawn_id, result);
    expression_set_integer(info->scene_variable, result->entity_id);

    return result;
}

void overworld_check_tile_spawns(struct overworld* overworld, struct overworld_actor_tile* tile, float tile_x, float tile_y, float radius) {
    int int_rad_sqrd = (int)(radius * radius * (256.0f * 256.0f));

    int x = (int)((tile_x - tile->x) * 256.0f);
    int y = (int)((tile_y - tile->y) * 256.0f);

    struct overworld_actor_spawn_location* end = tile->spawn_locations + tile->active_spawn_locations;

    for (
        struct overworld_actor_spawn_location* curr = tile->spawn_locations; 
        curr < end;
    ) {
        int dx = (int)curr->x - x;
        int dy = (int)curr->y - y;

        if (dx * dx + dy * dy > int_rad_sqrd) {
            ++curr;
            continue;
        }

        int spawn_id = (int)tile->first_spawn_id + (int)curr->spawn_id_offset;
        
        if (!hash_map_get(&overworld->loaded_actors, spawn_id)) {
            overworld_actor_spawn(overworld, tile, curr);
        }
        
        --end;
        *curr = *end;
    }

    tile->active_spawn_locations = end - tile->spawn_locations;
}

struct overworld_actor_tile** overworld_get_actor_tile_slot(struct overworld* overworld, int tile_x, int tile_y) {
    return &overworld->loaded_actor_tiles[tile_x & 0x1][tile_y & 0x1];
}

void overworld_reset_spawn_location(struct overworld* overworld, struct overworld_actor* actor) {
    struct overworld_actor_tile** slot = overworld_get_actor_tile_slot(overworld, actor->tile_x, actor->tile_y);

    if (*slot == NULL) {
        return;
    }

    struct overworld_actor_tile* tile = *slot;

    if (tile->x != actor->tile_x || tile->y != actor->tile_y) {
        return;
    }

    assert(tile->active_spawn_locations < tile->total_spawn_locations);

    struct overworld_actor_spawn_location* location = &tile->spawn_locations[tile->active_spawn_locations];
    location->x = actor->x;
    location->y = actor->y;
    location->spawn_id_offset = actor->spawn_id - tile->first_spawn_id;
    tile->active_spawn_locations += 1;
}

void overworld_check_actor_despawn(struct overworld* overworld, struct Vector3* player_pos) {
    struct overworld_actor* current = overworld->next_active_actor;
    struct overworld_actor* prev = NULL;

    while (current) {
        void* entity = entity_get(current->entity_id);

        bool should_remove;
        bool should_reset;

        if (!entity) {
            should_remove = true;
            should_reset = false;
        } else {
            // this is a bit hacky, right now all
            // entities need to put their position
            // first in the layout
            struct Vector3* entity_pos = entity;

            float dx = entity_pos->x - player_pos->x;
            float dz = entity_pos->z - player_pos->z;

            should_remove = dx * dx + dz * dz > DESPAWN_RADIUS * DESPAWN_RADIUS;
            should_reset = true;
        }

        struct overworld_actor* next = current->next;

        if (should_remove) {
            entity_despawn(current->entity_id);
            hash_map_delete(&overworld->loaded_actors, current->spawn_id);
            if (should_reset) {
                overworld_reset_spawn_location(overworld, current);
            }
            expression_set_integer(current->scene_variable, 0);
            if (prev) {
                prev->next = next;
            } else {
                overworld->next_active_actor = next;
            }
            current->next = overworld->next_free_actor;
            overworld->next_free_actor = current;
        }

        current = next;
    }
}

void overworld_check_collider_tiles(struct overworld* overworld, struct Vector3* player_pos) {
    float tile_x = (player_pos->x - overworld->min.x) * overworld->inv_tile_size;
    float tile_y = (player_pos->z - overworld->min.y) * overworld->inv_tile_size;
    float radius = SPAWN_IN_RADIUS * overworld->inv_tile_size;
    float collider_radius = COLLIDER_SPAWN_RADIUS * overworld->inv_tile_size;

    int min_x = (int)floorf(tile_x - collider_radius);
    int min_y = (int)floorf(tile_y - collider_radius);

    int max_x = (int)ceilf(tile_x + collider_radius);
    int max_y = (int)ceilf(tile_y + collider_radius);

    for (int x = min_x; x < max_x; x += 1) {
        for (int y = min_y; y < max_y; y += 1) {
            struct overworld_actor_tile** slot = overworld_get_actor_tile_slot(overworld, x, y);
        
            if (*slot) {
                if ((*slot)->x == x && (*slot)->y == y) {
                    // already loaded
                    overworld_check_tile_spawns(overworld, *slot, tile_x, tile_y, radius);
                    continue;;
                }
        
                collision_scene_remove_static_mesh(&(*slot)->collider);
                overworld_actor_tile_free(*slot);
            }
        
            if (x < 0 || y < 0 || x >= overworld->tile_x || y >= overworld->tile_y) {
                *slot = NULL;
                continue;
            }
            
            fseek(overworld->file, overworld->tile_definitions[x + y * overworld->tile_x].actor_block_offset, SEEK_SET);
            *slot = overworld_actor_tile_load(overworld->file);
            overworld_actor_tile_mark_spawned(overworld, *slot);
            (*slot)->x = x;
            (*slot)->y = y;
            collision_scene_add_static_mesh(&(*slot)->collider);
        }
    }

    overworld_check_actor_despawn(overworld, player_pos);
}