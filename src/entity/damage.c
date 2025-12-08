#include "damage.h"

#include <memory.h>

void damaged_set_reset(struct damaged_set* set) {
    memset(set, 0, sizeof(struct damaged_set));
}

bool damaged_set_check(struct damaged_set* set, entity_id id) {
    uint8_t shortened_id = (uint8_t)(id & 0xFF);

    if (!set) {
        return true;
    }

    uint8_t* max = set->damaged_entities + MAX_DAMAGED_SET_SIZE;

    for (uint8_t* it = set->damaged_entities; it < max; ++it) {
        if (*it == shortened_id) {
            return false;
        }

        if (*it == 0) {
            *it = shortened_id;
            return true;
        }
    }

    return true;
}