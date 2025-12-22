#ifndef __ENTITIES_REPAIR_INTERACTION_H__
#define __ENTITIES_REPAIR_INTERACTION_H__

#include "../scene/scene_definition.h"
#include "../entity/entity_id.h"
#include "../entity/interactable.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"

struct repair_interaction {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t obj;
};

typedef struct repair_interaction repair_interaction_t;

void repair_interaction_common_init();
void repair_interaction_common_destroy();

void repair_interaction_init(repair_interaction_t* repair_interaction, struct repair_interaction_definition* definition, entity_id entity_id);
void repair_interaction_destroy(repair_interaction_t* repair_interaction);

#endif