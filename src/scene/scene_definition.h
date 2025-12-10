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
    
    ENTITY_TYPE_count,
};

enum fixed_entity_ids {
    ENTITY_ID_PLAYER = 1,
    ENTITY_ID_SUBJECT = 2,

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

#endif