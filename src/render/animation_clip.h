#ifndef __ANIMATION_CLIP_H___
#define __ANIMATION_CLIP_H___

#include <stdint.h>

#include "armature_definition.h"

struct animation_used_attributes {
    uint8_t has_pos: 1;
    uint8_t has_rot: 1;
    uint8_t has_scale: 1;
};

struct animation_clip {
    char* name;

    uint16_t bone_count;
    uint16_t frame_count;
    uint16_t frames_per_second;
    uint16_t frame_size;

    uint16_t has_events: 1;
    uint16_t has_image_frames: 1;
    uint16_t has_prim_color: 1;
    uint16_t has_env_color: 1;

    uint32_t frames_rom_address;

    struct animation_used_attributes* used_bone_attributes;
};

typedef struct animation_clip animation_clip_t;

struct animation_set {
    uint16_t clip_count;
    uint16_t bone_count;
    struct animation_clip* clips;
};

struct animation_set* animation_set_load(const char* filename);
void annotation_clip_set_free(struct animation_set* animation_set);

struct animation_clip* animation_set_find_clip(struct animation_set* set, const char* clip_name);
float animation_clip_get_duration(struct animation_clip* clip);

#endif