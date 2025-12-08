#ifndef __SCENE_CAMERA_ANIMATION_H__
#define __SCENE_CAMERA_ANIMATION_H__

#include <stdint.h>
#include "../math/vector3.h"
#include "../math/vector2s16.h"

struct camera_animation_frame {
    struct Vector3 position;
    float rotation[3];
    float fov;
};

struct camera_animation {
    char* name;
    uint16_t frame_count;
    uint32_t rom_offset;
};

struct camera_animation_list {
    struct camera_animation* animations;
    uint16_t animation_count;
    uint32_t rom_location;
};

void camera_animation_list_init(struct camera_animation_list* list, int count, uint32_t rom_location);
void camera_animation_list_destroy(struct camera_animation_list* list);

struct camera_animation* camera_animation_lookup(struct camera_animation_list* list, const char* name);

#endif