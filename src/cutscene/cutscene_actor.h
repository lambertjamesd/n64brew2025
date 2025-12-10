#ifndef __CUTSCENE_CUTSCENE_ACTOR_H__
#define __CUTSCENE_CUTSCENE_ACTOR_H__

#include "../math/vector3.h"
#include "../scene/scene_definition.h"
#include "../math/transform_mixed.h"
#include "../render/animator.h"
#include "../render/animation_clip.h"
#include "../render/armature.h"
#include "../collision/dynamic_object.h"
#include "../render/renderable.h"
#include <stdbool.h>

enum actor_state {
    ACTOR_STATE_IDLE = 0,
    ACTOR_STATE_LOOKING = (1 << 0),
    ACTOR_STATE_MOVING = (1 << 1),
    ACTOR_STATE_SPACE = (1 << 2),
    ACTOR_STATE_ACTIVE = (1 << 3),
    ACTOR_STATE_ANIMATING = (1 << 4),
};

struct cutscene_actor_animations {
    struct animation_clip* idle;
    struct animation_clip* walk;
    struct animation_clip* run;
};

struct cutscene_actor_def {
    float eye_level;
    float move_speed;
    float run_speed;
    float run_threshold;
    float rotate_speed;
    struct dynamic_object_type collider;
    uint16_t collision_layers;
    uint16_t collision_group;
};

typedef struct cutscene_actor_def cutscene_actor_def_t;

struct cutscene_actor {
    struct TransformSingleAxis transform;
    struct dynamic_object collider;
    struct animation_set* animation_set;
    struct cutscene_actor_animations animations;
    struct animator animator;
    struct armature* armature;
    struct Vector3 target;
    float animate_speed;
    float move_speed;
    enum actor_state state;
    struct cutscene_actor_def* def;
    animator_events_t last_animator_events;
};

typedef struct cutscene_actor cutscene_actor_t;

void cutscene_actor_common_init();
void cutscene_actor_common_destroy();

void cutscene_actor_init(struct cutscene_actor* actor, struct cutscene_actor_def* def, entity_id entity_id, struct TransformSingleAxis* transform, struct armature* armature, char* animations_path);

void cutscene_actor_destroy(struct cutscene_actor* actor);
void cutscene_actor_reset();
struct cutscene_actor* cutscene_actor_find(entity_id entity_id);

void cutscene_actor_interact_with(struct cutscene_actor* actor, enum interaction_type interaction, struct Vector3* at);
void cutscene_actor_idle(struct cutscene_actor* actor);
void cutscene_actor_set_speed(struct cutscene_actor* actor, float speed);

bool cutscene_actor_is_moving(struct cutscene_actor* actor);

// return true if actor cutscene is active
bool cutscene_actor_update(struct cutscene_actor* actor);

void cutscene_actor_run_animation(struct cutscene_actor* actor, const char* name, bool loop);

static inline vector3_t* cutscene_actor_get_pos(cutscene_actor_t* actor) {
    return &actor->transform.position;
}

static inline vector2_t* cutscene_actor_get_rot(cutscene_actor_t* actor) {
    return &actor->transform.rotation;
}

static inline vector3_t* cutscene_actor_get_vel(cutscene_actor_t* actor) {
    return &actor->collider.velocity;
}

static inline bool cutscene_actor_is_touching(cutscene_actor_t* actor, entity_id other) {
    return dynamic_object_find_contact(&actor->collider, other) != NULL;
}

static inline void cutscene_actor_run_clip(cutscene_actor_t* actor, animation_clip_t* clip, float start_time, bool loop) {
    animator_run_clip(&actor->animator, clip, start_time, loop);
}

static inline animation_clip_t* cutscene_actor_find_clip(cutscene_actor_t* actor, const char* name) {
    return animation_set_find_clip(actor->animation_set, name);
}

static inline bool cutscene_actor_is_animating(cutscene_actor_t* actor) {
    return animator_is_running(&actor->animator);
}

static inline bool cutscene_actor_is_animating_clip(cutscene_actor_t* actor, animation_clip_t* clip) {
    return animator_is_running_clip(&actor->animator, clip);
}

static inline animator_events_t cutscene_actor_get_animator_events(cutscene_actor_t* actor) {
    return actor->animator.events;
}

static inline entity_id cutscene_actor_get_id(cutscene_actor_t* actor) {
    return actor->collider.entity_id;
}

#endif