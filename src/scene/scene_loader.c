#include "scene_loader.h"

#include <malloc.h>
#include <string.h>
#include <libdragon.h>
#include "../resource/mesh_collider.h"
#include "../resource/tmesh_cache.h"
#include "../resource/material_cache.h"
#include "../render/render_scene.h"
#include "../time/time.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/evaluation_context.h"
#include "../cutscene/expression_evaluate.h"
#include "../overworld/overworld_load.h"
#include "../util/memory_stream.h"
#include "../effects/area_title.h"
#include "../render/defs.h"

#include "../collision/collision_scene.h"

#include "../entity/entity_spawner.h"

// WRLD
#define EXPECTED_HEADER 0x57524C44

// EXPR
#define EXPECTED_CONDITION_HEADER 0x45585052

bool scene_load_check_condition(FILE* file) {
    struct expression expression;
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_CONDITION_HEADER);
    uint16_t byte_size;
    fread(&byte_size, 1, 2, file);
    char expression_program[byte_size];
    expression.expression_program = expression_program;
    fread(expression.expression_program, 1, byte_size, file);

    struct evaluation_context eval_context;
    evaluation_context_init(&eval_context, 0);

    expression_evaluate(&eval_context, &expression);

    int result = evaluation_context_pop(&eval_context);

    evaluation_context_destroy(&eval_context);

    return result != 0;
}

void scene_load_static_particles(scene_t* scene, int room_count, FILE* file) {
    scene->static_particles = static_particles_load(&scene->static_particles_count, file);

    scene->room_particle_ranges = malloc(sizeof(struct static_entity_range) * room_count);
    fread(scene->room_particle_ranges, sizeof(struct static_entity_range), room_count, file);
}

void scene_load_camera_animations(struct camera_animation_list* list, const char* filename, FILE* file) {
    uint16_t count;
    fread(&count, sizeof(count), 1, file);
    camera_animation_list_init(list, count, 0);

    int filename_len = strlen(filename);
    char modified_filename[filename_len + 1];
    strcpy(modified_filename, filename + strlen("rom:/"));
    strcpy(modified_filename + filename_len - 10, "sanim");

    list->rom_location = dfs_rom_addr(modified_filename);

    for (int i = 0; i < count; ++i) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        char* name = malloc(name_length + 1);
        fread(name, name_length, 1, file);
        name[name_length] = '\0';

        struct camera_animation* animation = &list->animations[i];

        animation->name = name;
        fread(&animation->frame_count, sizeof(uint16_t), 1, file);
        fread(&animation->rom_offset, sizeof(uint32_t), 1, file);
        animation->rom_offset += list->rom_location;
    }
}

room_entity_block_t* scene_load_entities(int room_count, FILE* file) {
    room_entity_block_t* room_entities = malloc(sizeof(room_entity_block_t) * room_count);

    for (int i = 0; i < room_count; i += 1) {
        room_entity_block_t* room = &room_entities[i];
        fread(&room->block_size, 2, 1, file);
        room->block = room->block_size ? malloc(room->block_size) : NULL;
        fread(room->block, room->block_size, 1, file);

        fread(&room->shared_entity_count, 2, 1, file);
        room->shared_entity_index = room->shared_entity_count ? malloc(room->shared_entity_count * sizeof(uint16_t)) : NULL;
        fread(room->shared_entity_index, sizeof(uint16_t), room->shared_entity_count, file);
    }

    return room_entities;
}

void scene_load_shared_entities(shared_entity_block_t* shared_entities, FILE* file) {
    fread(&shared_entities->shared_entity_count, 2, 1, file);
    fread(&shared_entities->block_size, 2, 1, file);
    shared_entities->block = malloc(shared_entities->block_size);
    shared_entities->entities = malloc(sizeof(shared_room_entity_t) * shared_entities->shared_entity_count);

    fread(shared_entities->block, shared_entities->block_size, 1, file);

    memory_stream_t stream;
    memory_stream_init(&stream, shared_entities->block, shared_entities->block_size);
    
    uint16_t entity_count;
    memory_stream_read(&stream, &entity_count, 2); 
    assert(entity_count == shared_entities->shared_entity_count);

    shared_room_entity_t* entity = shared_entities->entities;

    for (int i = 0; i < entity_count; i += 1) {
        entity->block = memory_stream_curr(&stream);
        entity->entity_id = 0;
        entity->ref_count = 0;

        uint32_t expression_header;
        memory_stream_read(&stream, &expression_header, sizeof(uint32_t));
        assert(expression_header == EXPECTED_EXPR_HEADER);

        uint16_t expression_size;
        memory_stream_read(&stream, &expression_size, sizeof(uint16_t));
        memory_stream_read(&stream, NULL, expression_size);

        memory_stream_read(&stream, NULL, sizeof(uint16_t)); // on_despawn
        memory_stream_read(&stream, NULL, sizeof(uint16_t)); // script_location

        memory_stream_read(&stream, NULL, sizeof(uint16_t)); // entity_type
        uint16_t def_size;
        memory_stream_read(&stream, &def_size, sizeof(uint16_t));
        memory_stream_read(&stream, NULL, def_size);

        entity->block_size = (const char*)memory_stream_curr(&stream) - (const char*)entity->block;

        ++entity;
    }
}

void scene_load_loading_zones(struct scene* scene, FILE* file) {
    fread(&scene->loading_zone_count, 2, 1, file);

    scene->loading_zones = malloc(sizeof(struct loading_zone) * scene->loading_zone_count);
    fread(scene->loading_zones, sizeof(struct loading_zone), scene->loading_zone_count, file);

    for (int i = 0; i < scene->loading_zone_count; i += 1) {
        scene->loading_zones[i].scene_name += (int)scene->string_table;
    }
}

void scene_load_cutscene(struct scene* scene, FILE* file) {
    uint8_t cutscene_name_length;
    fread(&cutscene_name_length, 1, 1, file);

    if (cutscene_name_length) {
        char cutscene_name[cutscene_name_length + 1];
        fread(cutscene_name, 1, cutscene_name_length, file);
        cutscene_name[cutscene_name_length] = '\0';
        scene->cutscene = cutscene_load(cutscene_name);
    } else {
        scene->cutscene = NULL;
    }

    if (scene->room_count) {
        scene->room_cutscene_functions = malloc(sizeof(uint16_t) * scene->room_count);
        fread(scene->room_cutscene_functions, sizeof(uint16_t), scene->room_count, file);
    } else {
        scene->room_cutscene_functions = NULL;
    }

    uint16_t scene_var_length;
    fread(&scene_var_length, 2, 1, file);
    void* scene_vars = malloc(scene_var_length);
    fread(scene_vars, scene_var_length, 1, file);
    scene->scene_vars = scene_vars;
    expression_set_scene_variables(scene->scene_vars);
}

struct scene* scene_load(const char* filename) {
    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    struct scene* scene = malloc(sizeof(struct scene));

    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        scene->loaded_rooms[i].room_index = ROOM_INDEX_NONE;
    }

    scene->next_loaded_room_cutscene = 0;
    scene->is_running_step = false;

    struct player_definition player_def;
    player_def.location = gZeroVec;
    player_def.rotation = gRight2;

    uint8_t location_count;
    fread(&location_count, 1, 1, file);

    bool found_entry = false;

    struct cutscene* starting_cutscene = NULL;

    struct named_location* named_locations = malloc(sizeof(struct named_location) * location_count);
    struct named_location* end = named_locations + location_count;

    uint16_t current_room = 0;
    
    for (struct named_location* curr = named_locations; curr < end; ++curr) {
        uint8_t name_length;
        fread(&name_length, 1, 1, file);
        curr->name = malloc(name_length + 1);
        fread(curr->name, name_length, 1, file);
        curr->name[name_length] = '\0';

        uint8_t on_enter_length;
        fread(&on_enter_length, 1, 1, file);
        char on_enter[on_enter_length + 1];
        fread(on_enter, on_enter_length, 1, file);
        on_enter[on_enter_length] = '\0';

        fread(&curr->position, sizeof(struct Vector3), 1, file);
        fread(&curr->rotation, sizeof(struct Vector2), 1, file);
        fread(&curr->room_index, sizeof(uint16_t), 1, file);

        if (found_entry) {
            continue;
        }

        if (strcmp(curr->name, "default") == 0) {
            player_def.location = curr->position;
            player_def.rotation = curr->rotation;
            current_room = curr->room_index;
        }

        if (strcmp(curr->name, scene_get_next_entry()) == 0) {
            player_def.location = curr->position;
            player_def.rotation = curr->rotation;
            current_room = curr->room_index;
            found_entry = true;

            if (on_enter_length) {
                starting_cutscene = cutscene_load(on_enter);
            }
        }
    }

    scene->named_locations = named_locations;
    scene->named_location_count = location_count;
    scene->last_despawn_check = 0;

    cutscene_actor_common_init();
    camera_init(&scene->camera, DEFAULT_CAMERA_FOV, 1.0f, 125.0f);
    player_init(&scene->player, &player_def, &scene->camera.transform);
    camera_controller_init(&scene->camera_controller, &scene->camera, &scene->player);
    hud_init(&scene->hud, &scene->player, &scene->camera);

    uint16_t static_count;
    fread(&static_count, 2, 1, file);
    scene->static_entity_count = static_count;

    scene->static_entities = malloc(sizeof(struct static_entity) * static_count);

    for (int i = 0; i < static_count; ++i) {
        uint8_t str_len;
        fread(&str_len, 1, 1, file);

        struct static_entity* entity = &scene->static_entities[i];
        tmesh_load(&scene->static_entities[i].tmesh, file);
    }

    uint16_t room_count;
    fread(&room_count, 2, 1, file);
    scene->room_count = room_count;

    scene->room_static_ranges = malloc(sizeof(struct static_entity_range) * room_count);
    fread(scene->room_static_ranges, sizeof(struct static_entity_range), room_count, file);

    scene_load_static_particles(scene, room_count, file);

    mesh_collider_load(&scene->mesh_collider, file);
    collision_scene_add_static_mesh(&scene->mesh_collider);

    uint16_t strings_length;
    fread(&strings_length, 2, 1, file);

    scene->string_table = malloc(strings_length);
    fread(scene->string_table, strings_length, 1, file);

    scene->room_entities = scene_load_entities(room_count, file);
    scene_load_shared_entities(&scene->shared_entities, file);

    scene_load_loading_zones(scene, file);

    uint8_t overworld_filename_length;
    fread(&overworld_filename_length, 1, 1, file);
    if (overworld_filename_length) {
        char overworld_filename[overworld_filename_length + 1];
        fread(overworld_filename, overworld_filename_length, 1, file);
        overworld_filename[overworld_filename_length] = '\0';
        scene->overworld = overworld_load(overworld_filename);
    } else {
        scene->overworld = NULL;
    }

    scene_load_camera_animations(&scene->camera_animations, filename, file);

    scene_load_cutscene(scene, file);

    fclose(file);

    render_scene_add(NULL, 0.0f, scene_render, scene);
    update_add(scene, scene_update, UPDATE_PRIORITY_CAMERA, UPDATE_LAYER_WORLD);

    scene_show_room(scene, current_room);

    if (starting_cutscene) {
        cutscene_runner_run(starting_cutscene, 0, cutscene_runner_free_on_finish(), NULL, 0);
    }

    return scene;
}

void scene_release_particles(scene_t* scene) {
    static_particles_release(scene->static_particles, scene->static_particles_count);
    free(scene->room_particle_ranges);
}

void scene_release_room_entities(room_entity_block_t* room_entities, int room_count) {
    for (int i = 0; i < room_count; i += 1) {
        free(room_entities[i].block);
        free(room_entities[i].shared_entity_index);
    }

    free(room_entities);
}

void scene_release_shared_entities(shared_entity_block_t* entities) {
    for (int i = 0; i < entities->shared_entity_count; i += 1) {
        entity_despawn(entities->entities[i].entity_id);
    }
    free(entities->block);
    free(entities->entities);
}

void scene_release(struct scene* scene) {
    if (!scene) {
        return;
    }

    for (int i = 0; i < MAX_LOADED_ROOM; i += 1) {
        loaded_room_t* room = &scene->loaded_rooms[i];

        if (room->room_index != ROOM_INDEX_NONE) {
            scene_hide_room(scene, room->room_index);
        }
    }

    scene_release_room_entities(scene->room_entities, scene->room_count);  
    scene_release_shared_entities(&scene->shared_entities);  

    for (int i = 0; i < scene->static_entity_count; ++i) {
        struct static_entity* entity = &scene->static_entities[i];
        tmesh_release(&entity->tmesh);
    }
    free(scene->static_entities);
    free(scene->room_static_ranges);

    scene_release_particles(scene);

    render_scene_remove(scene);
    update_remove(scene);

    player_destroy(&scene->player);
    camera_controller_destroy(&scene->camera_controller);
    hud_destroy(&scene->hud);

    cutscene_actor_common_destroy();

    collision_scene_remove_static_mesh(&scene->mesh_collider);
    mesh_collider_release(&scene->mesh_collider);

    if (scene->overworld) {
        overworld_free(scene->overworld);
    }
    
    free(scene->string_table);
    free(scene->loading_zones);

    for (int i = 0; i < scene->named_location_count; i += 1) {
        free(scene->named_locations[i].name);
    }
    free(scene->named_locations);

    camera_animation_list_destroy(&scene->camera_animations);

    if (scene->cutscene) {
        cutscene_free(scene->cutscene);
    }
    free(scene->scene_vars);
    expression_set_scene_variables(NULL);

    free(scene);

    entity_despawn_all();
}