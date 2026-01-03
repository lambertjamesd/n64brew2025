#include "entity_spawner.h"

#include "../scene/scene_definition.h"
#include "../util/hash_map.h"
#include "../objects/empty.h"
#include "../objects/trigger_cube.h"
#include "../entities/motorcycle.h"
#include "../entities/repair_interaction.h"
#include "../entities/repair_part_pickup.h"
#include "../entities/door.h"
#include "../entities/npc.h"
#include "../entities/script_runner.h"
// include_list insert point

#define ENTITY_DEFINITION(name, fields) [ENTITY_TYPE_ ## name] = { \
    #name, \
    (entity_init)name ## _init, \
    (entity_destroy)name ## _destroy, \
    name ## _common_init, \
    name ## _common_destroy, \
    sizeof(struct name), \
    sizeof(struct name ## _definition), \
    fields, \
    sizeof(fields) / sizeof(*fields), \
    ENTITY_TYPE_ ## name \
}

static struct entity_field_type_location fields_empty[] = {};

static struct entity_field_type_location fields_repair_interaction[] = {
    { .offset = offsetof(struct repair_interaction_definition, repair_scene), .type = ENTITY_FIELD_TYPE_STRING },
};

static struct entity_field_type_location fields_npc[] = {
    { .offset = offsetof(struct npc_definition, dialog), .type = ENTITY_FIELD_TYPE_STRING },
};

static struct entity_field_type_location fields_script_runner[] = {
    { .offset = offsetof(struct script_runner_definition, target), .type = ENTITY_FIELD_TYPE_STRING },
};

static struct entity_definition scene_entity_definitions[ENTITY_TYPE_count] = {
    ENTITY_DEFINITION(empty, fields_empty),
    ENTITY_DEFINITION(trigger_cube, fields_empty),
    ENTITY_DEFINITION(motorcycle, fields_empty),
    ENTITY_DEFINITION(repair_interaction, fields_repair_interaction),
    ENTITY_DEFINITION(repair_part_pickup, fields_empty),
    ENTITY_DEFINITION(door, fields_empty),
    ENTITY_DEFINITION(npc, fields_npc),
    ENTITY_DEFINITION(script_runner, fields_script_runner),
    // scene_entity_definitions insert point
};

static uint16_t scene_entity_count[ENTITY_TYPE_count];

struct entity_definition* entity_find_def(const char* name) {
   for (int i = 0; i < sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions); i += 1) {
        struct entity_definition* def = &scene_entity_definitions[i];

        if (strcmp(name, def->name) == 0) {
            return def;
        }
   }

   return NULL;
}

struct entity_definition* entity_def_get(unsigned index) {
    if (index >= sizeof(scene_entity_definitions) / sizeof(*scene_entity_definitions)) {
        return NULL;
    }

    return &scene_entity_definitions[index];
}

static struct hash_map entity_mapping;
static entity_id last_despwned_id;

#define ENTITY_STARTING_CAPACITY    32

struct entity_header {
    struct entity_definition* entity_def;
    entity_id id;
};

void entity_add_reference(enum entity_type_id entity_type) {
    if (!scene_entity_count[entity_type]) {
        scene_entity_definitions[entity_type].common_init();
    }
    ++scene_entity_count[entity_type];
}

entity_id entity_spawn(enum entity_type_id type, void* definition) {
    struct entity_definition* entity_def = entity_def_get(type);

    if (!entity_def) {
        return 0;
    }

    if (!entity_mapping.entries) {
        if (!hash_map_init(&entity_mapping, ENTITY_STARTING_CAPACITY)) {
            return 0;
        }
    }
   
    entity_id result = entity_id_new();
    void* entity = malloc(entity_def->entity_size + sizeof(struct entity_header));
    
    if (!entity || !hash_map_set(&entity_mapping, result, entity)) {
        free(entity);
        return 0;
    }

    entity_add_reference(type);

    struct entity_header* header = entity;
    header->entity_def = entity_def;
    header->id = result; 

    entity_def->init(header + 1, definition, result);
    return result;
}

void entity_remove_reference(enum entity_type_id entity_type) {
    --scene_entity_count[entity_type];
    if (!scene_entity_count[entity_type]) {
        scene_entity_definitions[entity_type].common_destroy();
    }
}

bool entity_despawn(entity_id entity_id) {
    if (!entity_id) {
        return false;
    }

    void* entity = hash_map_get(&entity_mapping, entity_id);

    if (!entity) {
        return false;
    }

    struct entity_header* header = entity;
    entity_remove_reference(header->entity_def->entity_type);

    header->entity_def->destroy(header + 1);
    free(entity);

    hash_map_delete(&entity_mapping, entity_id);
    last_despwned_id = entity_id;

    return true;
}

void entity_despawn_all() {
    for (struct hash_map_entry* entry = hash_map_next(&entity_mapping, NULL); entry; entry = hash_map_next(&entity_mapping, entry)) {
        struct entity_header* header = entry->value;
        header->entity_def->destroy(header + 1);
        free(entry->value);   
    }

    hash_map_destroy(&entity_mapping);
    last_despwned_id = 0;
}

void* entity_get(entity_id entity_id) {
    if (!entity_id) {
        return NULL;
    }

    struct entity_header* header = hash_map_get(&entity_mapping, entity_id);

    if (!header) {
        return NULL;
    }

    return header + 1;
}

entity_id entity_get_last_despawned() {
    return last_despwned_id;
}