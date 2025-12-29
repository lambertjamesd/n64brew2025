#ifndef __SCENE_SCENE_DEFINITION_H__
#define __SCENE_SCENE_DEFINITION_H__

#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/quaternion.h"
#include "../cutscene/expression.h"
#include <stdint.h>
#include <stdbool.h>

enum entity_type_id {
    ENTITY_TYPE_empty,
    ENTITY_TYPE_trigger_cube,
    ENTITY_TYPE_motorcycle,
    ENTITY_TYPE_repair_interaction,
    ENTITY_TYPE_repair_part_pickup,
    ENTITY_TYPE_door,
    ENTITY_TYPE_npc,
    ENTITY_TYPE_script_runner,
    
    ENTITY_TYPE_count,
};

enum fixed_entity_ids {
    ENTITY_ID_PLAYER = 1,
    ENTITY_ID_SUBJECT = 2,
    ENTITY_ID_MOTORCYLE = 3,

    ENTITY_ID_FIRST_DYNAMIC,
};

struct empty_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct motorcycle_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

struct motorcycle_spawn_point_definition {
    struct Vector3 position;
    struct Vector2 rotation;
};

enum inventory_item_type {
    ITEM_TYPE_NONE,

    ITEM_TYPE_COUNT,
};

#define ROOM_NONE 0xFFFF

typedef uint16_t room_id;

typedef uint32_t entity_spawner;

#define VARIABLE_DISCONNECTED   0xFFFF
#define SCENE_VARIABLE_FLAG 0x8000
#define INT_SIZE_MASK       0x6000
#define INT_OFFSET_MASK     0x1FFF

#define GET_INT_VAR_SIZE(var)   (data_type_t)(((var) & INT_SIZE_MASK) >> 13)

typedef uint16_t boolean_variable;
typedef uint16_t integer_variable;

typedef char* script_location;
typedef char* scene_entry_point;

enum interaction_type {
    INTERACTION_NONE,

    INTERACTION_LOOK,
    INTERACTION_MOVE,
    INTERACTION_LOOK_MOVE,
    INTERACTION_SPACE,
    INTERACTION_LOOK_SPACE,
    INTERACTION_MOVE_SPACE,
    INTERACTION_LOOK_MOVE_SPACE,
};

enum fade_colors {
    FADE_COLOR_NONE,
    FADE_COLOR_BLACK,
    FADE_COLOR_WHITE,
};

struct trigger_cube_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    struct Vector3 scale;
};

struct repair_scene_definition {
    boolean_variable puzzle_complete;
    scene_entry_point exit_scene;
};

struct repair_part_definition {
    boolean_variable has_part;
};

enum repair_type {
    REPAIR_BIKE_MOTOR,
    REPAIR_WELL,
    REPAIR_COUNT,
};

struct repair_interaction_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum repair_type repair_type;
    scene_entry_point repair_scene;
};

enum repair_part_type {
    REPAIR_PART_MOTOR,
    REPAIR_PART_WATER_PUMP_GEAR,
    REPAIR_PART_WATER_PUMP_ARM,
    REPAIR_PART_COUNT,
};

struct repair_part_pickup_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum repair_part_type part_type;
    boolean_variable has_part;
};

enum door_type {
    DOOR_TYPE_GARAGE,
    DOOR_TYPE_COUNT,
};

struct door_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum door_type door_type;
    boolean_variable is_open;

    room_id room_a;
    room_id room_b;
};

enum npc_type {
    NPC_TYPE_SCRAPBOT1,
};

struct npc_definition {
    struct Vector3 position;
    struct Vector2 rotation;
    enum npc_type npc_type;
    script_location dialog;
};

struct script_runner_definition {
    struct Vector3 position;
    script_location target;
    bool loop;
};

#endif