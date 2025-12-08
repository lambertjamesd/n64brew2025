#ifndef __RENDER_ANIMATIONS_H__
#define __RENDER_ANIMATIONS_H__

#include <stdint.h>
#include <stdbool.h>
#include "armature_definition.h"
#include "animation_clip.h"
#include "armature.h"
#include "../math/transform.h"

union animator_events {
    struct {
        uint16_t reserved: 14;
        uint16_t step: 1;
        uint16_t attack: 1;
    };
    uint16_t all;
};

typedef union animator_events animator_events_t;

struct animator {
    struct animation_clip* current_clip;
    int16_t* bone_state[2];
    float current_time;
    float blend_lerp;
    uint16_t bone_state_frames[2];
    uint16_t next_frame_state_index;
    uint16_t bone_count;
    // flags
    uint16_t loop: 1;
    uint16_t done: 1;
    animator_events_t events;
    uint8_t image_frame_0;
    uint8_t image_frame_1;
    color_t prim_color;
    color_t env_color;
};

void animator_init(struct animator* animator, int bone_count);
void animator_destroy(struct animator* animator);
void animator_update(struct animator* animator, struct armature* armature, float delta_time);
void animator_run_clip(struct animator* animator, struct animation_clip* clip, float start_time, bool loop);
int animator_is_running(struct animator* animator);
bool animator_is_running_clip(struct animator* animator, struct animation_clip* clip);
float animator_get_time(struct animator* animator);

#endif