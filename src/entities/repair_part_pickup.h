#ifndef __ENTITIES_REPAIR_PART_PICKUP_H__
#define __ENTITIES_REPAIR_PART_PICKUP_H__

#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../collision/dynamic_object.h"
#include "../scene/scene_definition.h"
#include "../entity/interactable.h"
#include "../entity/entity_id.h"

struct repair_part_pickup {
    transform_sa_t transform;
    renderable_t renderable;
    dynamic_object_t collider;
    interactable_t interactable;
    boolean_variable has_part;
    enum repair_part_type part_type;
    float beep_timer;
    bool is_active;
    bool has_tracker;
};

typedef struct repair_part_pickup repair_part_pickup_t;

void repair_part_pickup_common_init();
void repair_part_pickup_common_destroy();

void repair_part_pickup_init(repair_part_pickup_t* repair_part_pickup, struct repair_part_pickup_definition* definition, entity_id entity_id);
void repair_part_pickup_destroy(repair_part_pickup_t* repair_part_pickup);

#endif