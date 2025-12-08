#ifndef __ENTITY_ENTITY_SPAWNER_H__
#define __ENTITY_ENTITY_SPAWNER_H__

#include "../scene/scene_definition.h"
#include "entity_id.h"

typedef void(*entity_init)(void* entity, void* definition, entity_id id);
typedef void(*entity_destroy)(void* entity);

typedef void(*entity_common_init)();
typedef void(*entity_common_destroy)();

struct entity_field_type_location {
    uint8_t type;
    uint8_t offset;
};

enum entity_field_types {
    ENTITY_FIELD_TYPE_STRING,
};

struct entity_definition {
    const char* name;
    entity_init init;
    entity_destroy destroy;
    entity_common_init common_init;
    entity_common_destroy common_destroy;
    uint16_t entity_size;
    uint16_t definition_size;
    struct entity_field_type_location* fields;
    uint16_t field_count;
    uint16_t entity_type;
};


entity_id entity_spawn(enum entity_type_id type, void* definition);
bool entity_despawn(entity_id entity_id);
void entity_despawn_all();
void* entity_get(entity_id entity_id);

entity_id entity_get_last_despawned();

struct entity_definition* entity_find_def(const char* name);
struct entity_definition* entity_def_get(unsigned index);

#endif