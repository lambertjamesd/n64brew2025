#ifndef __COLLISION_DYNAMIC_OBJECT_H__
#define __COLLISION_DYNAMIC_OBJECT_H__

#include "../entity/entity_id.h"
#include "../math/vector3.h"
#include "../math/box3d.h"
#include "../math/box2d.h"
#include "./shapes/swing_shape.h"
#include "contact.h"
#include "gjk.h"
#include <stdint.h>
#include <stdbool.h>

#define GRAVITY_CONSTANT    -9.8f

enum collision_layers {
    COLLISION_LAYER_TANGIBLE = (1 << 0),
    COLLISION_LAYER_LIGHTING_TANGIBLE = (1 << 1),
    COLLISION_LAYER_DAMAGE_PLAYER = (1 << 2),
    COLLISION_LAYER_DAMAGE_ENEMY = (1 << 3),
    COLLISION_LAYER_Z_TARGET = (1 << 4),
    COLLISION_LAYER_LIGHTNING_BALL = (1 << 5),
};

enum collision_group {
    COLLISION_GROUP_PLAYER = 1,
};

typedef void (*bounding_box_calculator)(void* data, struct Vector2* rotation, struct Box3D* box);

union dynamic_object_type_data
{
    struct { float radius; } sphere;
    struct { float radius; float inner_half_height; } capsule;
    struct { struct Vector3 half_size; } box;
    struct { struct Vector3 size; } cone;
    struct { float radius; float half_height; } cylinder;
    struct { struct Vector2 range; float radius; float half_height; } sweep;
    struct { struct swing_shape* shape; } swing;
};

struct dynamic_object_type {
    MinkowsiSum minkowsi_sum;
    bounding_box_calculator bounding_box;
    union dynamic_object_type_data data;
    float bounce;
    float friction;
    // 0 wont be stable on any slope, 1 will stick to anything not facing downward
    // or 1 - cos(slope angle)
    float max_stable_slope;
    uint16_t surface_type;
};

typedef struct dynamic_object_type dynamic_object_type_t;

#define DYNAMIC_OBJECT_MARK_PUSHED(object)              (object)->is_pushed = 2
#define DYNAMIC_OBJECT_MARK_DISABLE_FRICTION(object)    (object)->disable_friction = 2
#define DYNAMIC_OBJECT_MARK_JUMPING(object)             (object)->is_jumping = 2
#define DYNAMIC_OBJECT_MARK_UNDER_WATER(object)         (object)->under_water = 2
#define DYNAMIC_OBJECT_MARK_ICE_DASH(object)            (object)->has_ice_dash = 2

enum dynamic_density_class {
    DYNAMIC_DENSITY_LIGHT,
    DYNAMIC_DENSITY_MEDIUM,
    DYNAMIC_DENSITY_NEUTRAL,
    DYNAMIC_DENSITY_HEAVY,
};

enum dynamic_object_trigger_type {
    TRIGGER_TYPE_NONE,
    TRIGGER_TYPE_BASIC,
    TRIGGER_TYPE_OVERLAP,
};

enum weight_class {
    WEIGHT_CLASS_GHOST,
    WEIGHT_CLASS_LIGHT,
    WEIGHT_CLASS_MEDIUM,
    WEIGHT_CLASS_HEAVY,
    WEIGHT_CLASS_SUPER_HEAVY,
};

struct dynamic_object {
    entity_id entity_id;
    struct dynamic_object_type* type;
    struct Vector3* position;
    struct Vector2* rotation;
    struct Vector2* pitch;
    float scale;
    struct Vector3 center;
    struct Vector3 velocity;
    struct Box3D bounding_box;
    float time_scalar;
    uint16_t has_gravity: 1;
    uint16_t trigger_type: 2;
    uint16_t is_fixed: 1;
    uint16_t is_pushed: 2;
    uint16_t is_jumping: 2;
    uint16_t disable_friction: 2;
    uint16_t under_water: 2;
    uint16_t has_ice_dash: 2;
    uint16_t density_class: 2;
    uint16_t weight_class: 3;
    uint16_t hit_kill_plane: 1;
    uint16_t should_sweep_collide: 1;
    uint16_t collision_layers;
    uint16_t collision_group;
    struct contact* active_contacts;
    struct contact* shadow_contact;
};

typedef struct dynamic_object dynamic_object_t;

void dynamic_object_init(
    entity_id entity_id,
    struct dynamic_object* object, 
    struct dynamic_object_type* type,
    uint16_t collision_layers,
    struct Vector3* position, 
    struct Vector2* rotation
);

void dynamic_object_set_type(struct dynamic_object* object, struct dynamic_object_type* type);

void dynamic_object_update(struct dynamic_object* object);

struct contact* dynamic_object_nearest_contact(struct contact* first_contact, struct Vector3* position);
struct contact* dynamic_object_find_contact(struct dynamic_object* object, entity_id id);

void dynamic_object_minkowski_sum(void* data, struct Vector3* direction, struct Vector3* output);
void dynamic_object_recalc_bb(struct dynamic_object* object);

bool dynamic_object_should_slide(float max_stable_slope, float normal_y, enum surface_type surface_type);

bool dynamic_object_is_grounded(struct dynamic_object* object);
struct contact* dynamic_object_get_ground(struct dynamic_object* object);

void dynamic_object_set_scale(struct dynamic_object* object, float scale);

#define DYNAMIC_OBJECT_NEEDS_OVERLAP(obj)   ((obj)->trigger_type != TRIGGER_TYPE_BASIC)
#define DYNAMIC_OBJECT_SHOULD_PUSH(obj)     ((obj)->trigger_type == TRIGGER_TYPE_NONE)

#endif