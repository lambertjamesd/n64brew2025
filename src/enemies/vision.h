#ifndef __ENEMIES_VISION_H__
#define __ENEMIES_VISION_H__

#include "../entity/entity_id.h"
#include "../collision/spatial_trigger.h"
#include "../collision/dynamic_object.h"

struct dynamic_object* vision_update_current_target(entity_id* current_target_ptr, struct spatial_trigger* trigger, float sight_range, struct Vector3* offset);

#endif