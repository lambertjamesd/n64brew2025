#include "health_shield.h"

void health_shield_init(struct health_shield* health_shield, struct Vector3* direction, health_shield_block_callback block_callback, void* data) {
    health_shield->direction = *direction;
    health_shield->block_callback = block_callback;
    health_shield->data = data;
}

bool health_shield_does_block(struct health_shield* health_shield, struct damage_info* damage) {
    if (health_shield->block_callback && !health_shield->block_callback(health_shield->data, damage)) {
        return false;
    }

    return vector3Dot(&health_shield->direction, &damage->direction) < 0.0f;
}