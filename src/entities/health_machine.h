#ifndef __ENTITIES_HEALTH_MACHINE_H__
#define __ENTITIES_HEALTH_MACHINE_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../math/transform_single_axis.h"
#include "../entity/interactable.h"

struct health_machine {
    entity_id entity_id;
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    interactable_t interactable;
};

typedef struct health_machine health_machine_t;

void health_machine_init(health_machine_t* health_machine, struct health_machine_definition* definition, entity_id entity_id);
void health_machine_destroy(health_machine_t* health_machine, struct health_machine_definition* definition);
void health_machine_common_init();
void health_machine_common_destroy();

#endif