#ifndef __SCENE_PLAYER_H__
#define __SCENE_PLAYER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector2.h"
#include "../render/render_batch.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../collision/spatial_trigger.h"
#include "../render/animation_clip.h"
#include "../render/animator.h"
#include "../cutscene/cutscene_actor.h"

#include "../entity/health.h"
#include "../entity/interactable.h"

#define PLAYER_CAST_SOURCE_COUNT    5
#define CLIMB_UP_COUNT              3

enum player_state {
    PLAYER_GROUNDED,
};

enum player_animation {
    PLAYER_ANIMATION_COUNT,
};

enum player_sound {
    PLAYER_SOUND_COUNT,
};

struct player_definition {
    struct Vector3 location;
    struct Vector2 rotation;
};

union state_data {
    struct {
        float timer;
        float y_velocity;
        struct Vector3 start_pos;
        struct Vector2 target_rotation;
        uint8_t climb_up_index;
    } climbing_up;
    struct {
        entity_id carrying;
        float carry_offset;
        bool should_carry;
    } carrying;
};

typedef union state_data state_data_t;

struct player {
    struct cutscene_actor cutscene_actor;
    struct renderable renderable;
    struct Transform* camera_transform;
    struct health health;
    spatial_trigger_t vision;
    
    animation_clip_t* animations[PLAYER_ANIMATION_COUNT];
    struct animation_clip* last_spell_animation;
    wav64_t* sounds[PLAYER_SOUND_COUNT];

    struct Vector3 last_good_footing;
    float slide_timer;
    float coyote_time;

    enum player_state state;
    union state_data state_data;
};

typedef struct player player_t;

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform);

void player_render(struct player* player, struct render_batch* batch);

void player_destroy(struct player* player);

struct Vector3* player_get_position(struct player* player);

#endif