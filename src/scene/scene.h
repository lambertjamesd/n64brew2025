#ifndef __SCENE_SCENE_H__
#define __SCENE_SCENE_H__

#include "../render/render_batch.h"
#include "../render/camera.h"
#include "../render/tmesh.h"
#include "../collision/mesh_collider.h"

#include "../player/player.h"
#include "../overworld/overworld.h"
#include "camera_controller.h"
#include "camera_animation.h"
#include "../entity/entity_spawner.h"
#include "../particles/static_particles.h"
#include "../cutscene/cutscene.h"
#include "../menu/hud.h"
#include "../audio/audio.h"

struct static_entity {
    struct tmesh tmesh;
};

struct loading_zone {
    struct Box3D bounding_box;
    char* scene_name;
};

struct named_location {
    char* name;
    struct Vector3 position;
    struct Vector2 rotation;
    uint16_t room_index;
};

struct static_entity_range {
    uint16_t start;
    uint16_t end;
};

struct room_entity_block {
    void* block;
    uint16_t* shared_entity_index;
    uint16_t block_size;
    uint16_t shared_entity_count;
};

typedef struct room_entity_block room_entity_block_t;

struct shared_room_entity {
    entity_id entity_id;
    uint16_t ref_count;
    const void* block;
    uint16_t block_size;
};

typedef struct shared_room_entity shared_room_entity_t;

struct shared_entity_block {
    void* block;
    shared_room_entity_t* entities;
    uint16_t block_size;
    uint16_t shared_entity_count;
};

typedef struct shared_entity_block shared_entity_block_t;

#define ROOM_INDEX_NONE         0xFFFF
#define FUNCTION_INDEX_NONE     0xFFFF

struct loaded_entity {
    entity_id id;
    boolean_variable on_despawn;
    integer_variable script_location;
    uint16_t entity_type;
    const void* definition;
};

typedef struct loaded_entity loaded_entity_t;

struct loaded_room {
    uint16_t room_index;
    uint16_t entity_count;
    loaded_entity_t* entities;
};

typedef struct loaded_room loaded_room_t;

#define MAX_LOADED_ROOM 4

#define NEXT_LOADED_ROOM(curr)  (((curr) + 1) % MAX_LOADED_ROOM)

struct scene {
    struct static_entity* static_entities;
    struct static_entity_range* room_static_ranges;
    static_particles_t* static_particles;
    struct static_entity_range* room_particle_ranges;

    struct mesh_collider mesh_collider;

    struct player player;
    struct Camera camera;
    struct camera_controller camera_controller;
    hud_t hud;
    
    struct loading_zone* loading_zones;
    struct named_location* named_locations;
    struct overworld* overworld;
    vector2_t minimap_min;
    vector2_t minimap_max;

    room_entity_block_t* room_entities;
    shared_entity_block_t shared_entities;

    uint16_t room_count;
    uint16_t static_entity_count;
    uint16_t static_particles_count;
    uint16_t loading_zone_count;
    uint16_t named_location_count;
    uint8_t next_loaded_room_cutscene;
    bool is_running_step;

    entity_id last_despawn_check;

    loaded_room_t loaded_rooms[MAX_LOADED_ROOM];

    cutscene_t* cutscene;
    uint16_t* room_cutscene_functions;
    char* string_table;
    char* scene_vars;
    bool can_pause;

    wav64_t* music;
    audio_id music_id;

    struct camera_animation_list camera_animations;
};

typedef struct scene scene_t;

extern struct scene* current_scene;

void scene_render(void* data, struct render_batch* batch);
void scene_update(void* data);

void scene_queue_next(char* scene_name);
void scene_clear_next();

bool scene_show_room(struct scene* scene, int room_index);
void scene_hide_room(struct scene* scene, int room_index);

void scene_spawn_entity(struct scene* scene, int room_index, int entity_index);

bool scene_is_showing_room(struct scene* scene, int room_index);

bool scene_has_next();

char* scene_get_next();
char* scene_get_next_entry();

struct named_location* scene_find_location(char* name);

void scene_entity_apply_types(void* definition, char* string_table, struct entity_field_type_location* type_locations, int type_location_count);

#endif