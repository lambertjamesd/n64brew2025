#ifndef __COLLISION_SPATIAL_TRIGGER_H__
#define __COLLISION_SPATIAL_TRIGGER_H__

#include "../math/transform_single_axis.h"
#include "../math/vector3.h"
#include "../math/vector2.h"
#include "../math/box3d.h"
#include "../entity/entity_id.h"
#include "contact.h"
#include <stdbool.h>
#include <stdint.h>

enum SPATIAL_TRIGGER_TYPE {
    SPATIAL_TRIGGER_SPHERE,
    SPATIAL_TRIGGER_CYLINDER,
    SPATIAL_TRIGGER_BOX,
    SPATIAL_TRIGGER_WEDGE,
};

union spatial_trigger_data {
    struct { float radius; } sphere;
    struct { float radius; float half_height; } cylinder;
    struct { struct Vector3 half_size; } box;
    struct { float radius; float half_height; struct Vector2 angle; } wedge;
};

struct spatial_trigger_type {
    enum SPATIAL_TRIGGER_TYPE type;
    union spatial_trigger_data data;
    struct Vector3 center;
};

typedef struct spatial_trigger_type spatial_trigger_type_t;

struct spatial_trigger {
    struct TransformSingleAxis* transform;
    struct spatial_trigger_type* type;
    struct Box3D bounding_box;
    struct contact* active_contacts;
    uint16_t collision_layers;
    uint16_t collision_group;
    entity_id entity_id;
};

typedef struct spatial_trigger spatial_trigger_t;

#define SPATIAL_TRIGGER_SPHERE(radius)  .type = SPATIAL_TRIGGER_SPHERE, .data = {.sphere = {radius}}
#define SPATIAL_TRIGGER_CYLINDER(radius, half_height)   .type = SPATIAL_TRIGGER_CYLINDER, .data = {.cylinder = {radius, half_height}}
#define SPATIAL_TRIGGER_BOX(x, y, z) .type = SPATIAL_TRIGGER_BOX, .data = {.box = {.half_size = {x,y,z}}}
#define SPATIAL_TRIGGER_WEDGE(radius, half_height, angle_x, angle_y) .type = SPATIAL_TRIGGER_WEDGE, .data = {.wedge = {radius, half_height, {angle_x, angle_y}}}

void spatial_trigger_init(struct spatial_trigger* trigger, struct TransformSingleAxis* transform, struct spatial_trigger_type* type, uint16_t collision_layers, entity_id entity_id);

void spatial_trigger_recalc_bb(struct spatial_trigger* trigger);
void spatial_trigger_type_recalc_bb(struct spatial_trigger_type* type, struct TransformSingleAxis* transform, struct Box3D* result);
bool spatial_trigger_does_contain_point(struct spatial_trigger* trigger, struct Vector3* point);

#endif