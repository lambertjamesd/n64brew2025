#ifndef __COLLISION_CONTACT_H__
#define __COLLISION_CONTACT_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "surface_type.h"
#include <stdint.h>
#include <stdbool.h>

struct dynamic_object;

struct contact {
    struct contact* next;
    struct Vector3 point;
    struct Vector3 normal;
    entity_id other_object;
    enum surface_type surface_type;
    uint16_t collision_layers;
};

typedef struct contact contact_t;

bool contacts_are_touching(contact_t* first, entity_id other_object);

#endif