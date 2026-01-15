#ifndef __ENTITIES_CHECKPOINT_H__
#define __ENTITIES_CHECKPOINT_H__

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"
#include "../collision/spatial_trigger.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"

struct checkpoint {
    transform_sa_t transform;
    spatial_trigger_t trigger;
    renderable_t renderable;
};

typedef struct checkpoint checkpoint_t;

void checkpoint_init(checkpoint_t* checkpoint, struct checkpoint_definition* definition, entity_id entity_id);
void checkpoint_destroy(checkpoint_t* checkpoint, struct checkpoint_definition* definition);
void checkpoint_common_init();
void checkpoint_common_destroy();

#endif