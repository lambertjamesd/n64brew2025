#ifndef __CUTSCENE_CUTSCENE_H__
#define __CUTSCENE_CUTSCENE_H__

#include <stdint.h>
#include <stdbool.h>
#include "../scene/scene_definition.h"
#include "cutscene_actor.h"
#include "expression.h"

enum cutscene_step_type {
    CUTSCENE_STEP_DIALOG,
    CUTSCENE_STEP_SHOW_ITEM,
    CUTSCENE_STEP_PAUSE,
    CUTSCENE_STEP_EXPRESSION,
    CUTSCENE_STEP_JUMP_IF_NOT,
    CUTSCENE_STEP_JUMP,
    CUTSCENE_STEP_SET_LOCAL,
    CUTSCENE_STEP_SET_SCENE,
    CUTSCENE_STEP_SET_GLOBAL,
    CUTSCENE_STEP_DELAY,
    CUTSCENE_STEP_INTERACT_WITH_NPC,
    CUTSCENE_STEP_IDLE_NPC,
    CUTSCENE_STEP_CAMERA_LOOK_AT_NPC,
    CUTSCENE_STEP_CAMERA_FOLLOW,
    CUTSCENE_STEP_CAMERA_RETURN,
    CUTSCENE_STEP_CAMERA_ANIMATE,
    CUTSCENE_STEP_CAMERA_MOVE_TO,
    CUTSCENE_STEP_CAMERA_WAIT,
    CUTSCENE_STEP_INTERACT_WITH_LOCATION,
    CUTSCENE_STEP_FADE,
    CUTSCENE_STEP_INTERACT_WITH_POSITION,
    CUTSCENE_STEP_NPC_WAIT,
    CUTSCENE_STEP_NPC_SET_SPEED,
    CUTSCENE_STEP_SHOW_TITLE,
    CUTSCENE_STEP_LOOK_AT_SUBJECT,
    CUTSCENE_STEP_NPC_ANIMATE,
    CUTSCENE_STEP_PRINT,
    CUTSCENE_STEP_SPAWN,
    CUTSCENE_STEP_CALLBACK,
};

typedef void (*cutscene_step_callback)(void* data);

struct cutscene_step;

struct cutscene_function {
    struct cutscene_step* steps;
    uint16_t step_count;
    const char* name;
};

typedef struct cutscene_function cutscene_function_t;

struct cutscene {
    struct cutscene_step* steps;
    uint16_t step_count;

    uint16_t function_count;
    cutscene_function_t* functions;
    
    uint16_t locals_size;
    void* locals;
};

typedef struct cutscene cutscene_t;

struct templated_string {
    char* template;
    uint16_t nargs;
};

struct camera_move_to_args {
    bool instant: 1;
    bool move_target: 1; 
};

typedef struct camera_move_to_args camera_move_to_args_t;

union cutscene_step_data {
    struct {
        struct templated_string message;
    } dialog;
    struct {
        uint16_t item;
        uint16_t should_show;
    } show_item;
    struct {
        uint8_t should_pause;
        uint8_t should_change_game_mode;
        uint16_t layers;
    } pause;
    struct {
        uint16_t context_size;
    } push_context;
    struct {
        struct expression expression;
    } expression;
    struct {
        int offset;
    } jump;
    struct {
        uint16_t data_type;
        uint16_t word_offset;
    } store_variable;
    struct {
        float duration;
    } delay;
    struct {
        enum interaction_type type;
    } interact_with_npc;
    struct {
        char* animation_name;
    } camera_animate;
    struct {
        struct Vector3 target;
        camera_move_to_args_t args;
    } camera_move_to;
    struct {
        enum interaction_type type;
        char* location_name;
    } interact_with_location;
    struct {
        enum fade_colors color;
        float duration;
    } fade;
    struct {
        enum interaction_type type;
        struct Vector3 position;
    } interact_with_position;
    struct {
        float speed;
    } npc_set_speed;
    struct {
        char* message;
    } show_title;
    struct {
        char* animation_name;
        uint8_t loop;
    } npc_animate;
    struct {
        struct templated_string message;
    } print;
    struct {
        cutscene_step_callback callback;
        void* data;
    } callback;
};

struct cutscene_step {
    enum cutscene_step_type type;
    union cutscene_step_data data;
};

typedef struct cutscene_step cutscene_step_t;

struct cutscene* cutscene_new(int capacity, int locals_capacity, int function_count);
void cutscene_free(struct cutscene* cutscene);

#define MAX_BUILDER_STEP_COUNT  32

struct cutscene_builder {
    struct cutscene_step steps[MAX_BUILDER_STEP_COUNT];
    uint16_t step_count;
};

struct cutscene* cutscene_load(char* filename);
void cutscene_builder_init(struct cutscene_builder* builder);

void cutscene_builder_pause(struct cutscene_builder* builder, bool should_pause, bool should_change_game_mode, int layers);
void cutscene_builder_dialog(struct cutscene_builder* builder, const char* message);
void cutscene_builder_show_item(struct cutscene_builder* builder, enum inventory_item_type item, bool should_show);
void cutscene_builder_delay(struct cutscene_builder* builder, float delay);
void cutscene_builder_interact_npc(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    entity_id target
);
void cutscene_builder_npc_set_speed(struct cutscene_builder* builder, entity_id subject, float speed);
void cutscene_builder_interact_position(
    struct cutscene_builder* builder,
    enum interaction_type type,
    entity_id subject,
    struct Vector3* position
);
void cutscene_builder_npc_wait(
    struct cutscene_builder* builder,
    entity_id subject
);
void cutscene_builder_camera_wait(struct cutscene_builder* builder);
void cutscene_builder_camera_follow(struct cutscene_builder* builder);
void cutscene_builder_camera_return(struct cutscene_builder* builder);
void cutscene_builder_camera_move_to(struct cutscene_builder* builder, struct Vector3* position, camera_move_to_args_t* args);
void cutscene_builder_set_boolean(struct cutscene_builder* builder, boolean_variable variable, bool value);
void cutscene_builder_callback(struct cutscene_builder* builder, cutscene_step_callback callback, void* data);

void cutscene_builder_expression(struct cutscene_builder* builder, expression_builder_t* expression);

struct cutscene* cutscene_builder_finish(struct cutscene_builder* builder);

#endif