#ifndef __ENTITIES_DOOR_H__
#define __ENTITIES_DOOR_H__

#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../scene/scene_definition.h"
#include "../render/animator.h"
#include "../render/animation_clip.h"

struct door_animations {
    animation_clip_t* open;
    animation_clip_t* close;
};

typedef struct door_animations door_animations_t;

struct door {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    animator_t animator;
    door_animations_t animations;
    animation_set_t* animation_set;
    boolean_variable is_open;
    bool was_open;
};

typedef struct door door_t;

void door_common_init();
void door_common_destroy();

void door_init(door_t* door, struct door_definition* definition, entity_id entity_id);
void door_destroy(door_t* door);

#endif