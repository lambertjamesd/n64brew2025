#include "animator.h"

#include <libdragon.h>
#include <malloc.h>
#include "../math/transform.h"
#include "../math/mathf.h"

#define MAX_ANIMATION_QUEUE_ENTRIES 20

#define ALIGN_UP(size)      (((size) + 7) & ~7)

void animator_sync() {
    dma_wait();
}

void animator_init(struct animator* animator, int bone_count) {
    animator->current_clip = NULL;
    animator->current_time = 0.0f;
    animator->blend_lerp = 0.0f;
    int bone_state_size = ALIGN_UP(sizeof(struct armature_packed_transform) * bone_count + sizeof(uint16_t));
    if (bone_count) {
        animator->bone_state[0] = malloc(bone_state_size);
        animator->bone_state[1] = malloc(bone_state_size);
        data_cache_hit_writeback(animator->bone_state[0], bone_state_size);
        data_cache_hit_writeback(animator->bone_state[1], bone_state_size);
    } else {
        animator->bone_state[0] = NULL;
        animator->bone_state[1] = NULL;
    }
    animator->bone_state_frames[0] = -1;
    animator->bone_state_frames[1] = -1;
    animator->next_frame_state_index = -1;
    animator->bone_count = bone_count;
    animator->events.all = 0;
    animator->image_frame_0 = NO_IMAGE_FRAME;
    animator->image_frame_1 = NO_IMAGE_FRAME;
}

void animator_destroy(struct animator* animator) {
    free(animator->bone_state[0]);
    free(animator->bone_state[1]);

    animator->bone_state[0] = NULL;
    animator->bone_state[1] = NULL;
}

void animator_request_frame(struct animator* animator, int next_frame) {
    struct animation_clip* current_clip = animator->current_clip;

    if (!current_clip) {
        return;
    }

    if (next_frame < 0 || next_frame >= current_clip->frame_count) {
        return;
    }

    if (animator->next_frame_state_index == -1) {
        animator->next_frame_state_index = 0;
    }

    if (animator->bone_state_frames[animator->next_frame_state_index] == next_frame) {
        return;
    }

    animator->bone_state_frames[animator->next_frame_state_index] = next_frame;

    uint32_t start = (uint32_t)animator->bone_state[animator->next_frame_state_index];
    uint32_t end = start + current_clip->frame_size;

    start &= ~0xF;
    end = (end + 0xF) & ~0xF;

    data_cache_hit_invalidate((void*)start, end - start);
    dma_read(animator->bone_state[animator->next_frame_state_index], current_clip->frames_rom_address + current_clip->frame_size * next_frame, current_clip->frame_size);
}

int16_t* animator_extract_bone(int16_t* bone_data, struct animation_used_attributes attributes, struct Transform* result) {
    if (attributes.has_pos) {
        result->position.x = (float)(bone_data[0] * (1.0f / 8.0f));
        result->position.y = (float)(bone_data[1] * (1.0f / 8.0f));
        result->position.z = (float)(bone_data[2] * (1.0f / 8.0f));

        bone_data += 3;
    }

    if (attributes.has_rot) {
        quatUnpack(bone_data, &result->rotation);
        
        bone_data += 3;
    }

    if (attributes.has_scale) {
        result->scale.x = (float)(bone_data[0] * (1.0f / 256.0f));
        result->scale.y = (float)(bone_data[1] * (1.0f / 256.0f));
        result->scale.z = (float)(bone_data[2] * (1.0f / 256.0f));

        bone_data += 3;
    }

    return bone_data;
}

void animator_init_zero_transform(struct animator* animator, struct animation_used_attributes* used_attributes, struct Transform* transforms) {
    if (animator->next_frame_state_index == -1) {
        return;
    }

    for (int i = 0; i < animator->bone_count; ++i) {
        if (used_attributes->has_pos) {
            transforms[i].position = gZeroVec;
        }
        if (used_attributes->has_rot) {
            transforms[i].rotation = gQuaternionZero;
        }
        if (used_attributes->has_scale) {
            transforms[i].scale = gZeroVec;
        }

        used_attributes += 1;
    }

    animator->events.all = 0;
}

void animator_normalize(struct animator* animator, struct Transform* transforms) {
    for (int i = 0; i < animator->bone_count; ++i) {
        quatNormalize(&transforms[i].rotation, &transforms[i].rotation);
    }
}

void animator_blend_transform(struct animator* animator, int16_t* frame, struct animation_used_attributes* used_attributes, struct Transform* transforms, int bone_count, float weight) {
    for (int i = 0; i < bone_count; ++i) {
        struct Transform boneTransform;
        frame = animator_extract_bone(frame, *used_attributes, &boneTransform);
        
        if (used_attributes->has_pos) {
            vector3AddScaled(&transforms[i].position, &boneTransform.position, weight, &transforms[i].position);
        }

        if (used_attributes->has_rot) {
            if (quatDot(&transforms[i].rotation, &boneTransform.rotation) < 0) {
                transforms[i].rotation.x -= boneTransform.rotation.x * weight;
                transforms[i].rotation.y -= boneTransform.rotation.y * weight;
                transforms[i].rotation.z -= boneTransform.rotation.z * weight;
                transforms[i].rotation.w -= boneTransform.rotation.w * weight;
            } else {
                transforms[i].rotation.x += boneTransform.rotation.x * weight;
                transforms[i].rotation.y += boneTransform.rotation.y * weight;
                transforms[i].rotation.z += boneTransform.rotation.z * weight;
                transforms[i].rotation.w += boneTransform.rotation.w * weight;
            }
        }

        if (used_attributes->has_scale) {
            vector3AddScaled(&transforms[i].scale, &boneTransform.scale, weight, &transforms[i].scale);
        }

        used_attributes += 1;
    }

    if (animator->current_clip->has_events) {
        animator->events.all |= (uint16_t)*frame;
        frame += 1;
    }

    if (animator->current_clip->has_image_frames) {
        uint16_t frames = (uint16_t)*frame;
        animator->image_frame_0 = (uint8_t)(frames >> 8);
        animator->image_frame_1 = (uint8_t)(frames & 0xFF);
        frame += 1;
    }

    if (animator->current_clip->has_prim_color) {
        memcpy(&animator->prim_color, frame, sizeof(color_t));
        frame += 2;
    }

    if (animator->current_clip->has_env_color) {
        memcpy(&animator->env_color, frame, sizeof(color_t));
        frame += 2;
    }
}

void animator_read_transform_with_weight(struct animator* animator, struct Transform* transforms, float weight) {
    if (animator->blend_lerp >= 1.0f) {
        animator_blend_transform(animator, animator->bone_state[animator->next_frame_state_index], animator->current_clip->used_bone_attributes, transforms, animator->bone_count, weight);
        return;
    }

    animator_blend_transform(animator, animator->bone_state[animator->next_frame_state_index], animator->current_clip->used_bone_attributes, transforms, animator->bone_count, animator->blend_lerp * weight);
    animator_blend_transform(animator, animator->bone_state[animator->next_frame_state_index ^ 1], animator->current_clip->used_bone_attributes, transforms, animator->bone_count, (1.0f - animator->blend_lerp) * weight);
}

void animator_read_transform(struct animator* animator, struct Transform* transforms) {
    if (animator->next_frame_state_index == -1) {
        return;
    }

    animator_init_zero_transform(animator, animator->current_clip->used_bone_attributes, transforms);
    animator_read_transform_with_weight(animator, transforms, 1.0f);
    animator_normalize(animator, transforms);
}

int animator_bone_state_index_of_frame(struct animator* animator, int frame) {
    if (animator->bone_state_frames[0] == frame) {
        return 0;
    }

    if (animator->bone_state_frames[1] == frame) {
        return 1;
    }

    return -1;
}

int animator_clamp_frame(struct animator* animator, int frame) {
    if (frame < animator->current_clip->frame_count) {
        return frame;
    }

    if (animator->loop) {
        return frame - animator->current_clip->frame_count;
    }

    return animator->current_clip->frame_count - 1;
}

void animator_step(struct animator* animator, float delta_time) {
    struct animation_clip* current_clip = animator->current_clip;

    if (!current_clip) {
        return;
    }

    animator->current_time += delta_time;

    float duration = (float)current_clip->frame_count / (float)current_clip->frames_per_second;

    if ((animator->current_time >= duration && delta_time > 0.0f) || (animator->current_time < 0.0f && delta_time < 0.0f)) {
        if (animator->loop) {
            animator->current_time = duration ? mathfMod(animator->current_time, duration) : 0.0f;
        } else {
            animator->current_time = minf(duration, maxf(0.0f, animator->current_time));
            animator->done = 1;
        }
    }

    float currentFrameFractional = animator->current_time * current_clip->frames_per_second;
    int prevFrame = (int)floorf(currentFrameFractional);
    int next_frame = (int)ceilf(currentFrameFractional);
    float lerpValue = currentFrameFractional - prevFrame;

    prevFrame = animator_clamp_frame(animator, prevFrame);
    next_frame = animator_clamp_frame(animator, next_frame);

    if (next_frame == prevFrame) {
        lerpValue = 1.0f;
    }

    int existingPrevFrame = animator_bone_state_index_of_frame(animator, prevFrame);
    int existingNextFrame = animator_bone_state_index_of_frame(animator, next_frame);

    if (existingPrevFrame == -1 && existingNextFrame == -1) {
        // both frames need to be requested
        animator->blend_lerp = lerpValue;
        if (prevFrame != next_frame) {
            animator->next_frame_state_index = 0;
            animator_request_frame(animator, prevFrame);
        }
        animator->next_frame_state_index = 1;
        animator_request_frame(animator, next_frame);
        return;
    }

    if (existingNextFrame == -1) {
        // only the next frame needs to be requested
        animator->blend_lerp = lerpValue;
        animator->next_frame_state_index = existingPrevFrame ^ 1;
        animator_request_frame(animator, next_frame);
        return;
    }

    if (existingPrevFrame == -1) {
        // only the previous frame needs to be requested
        animator->blend_lerp = 1.0f - lerpValue;
        animator->next_frame_state_index = existingNextFrame ^ 1;
        animator_request_frame(animator, prevFrame);
        return;
    }

    if (existingNextFrame == existingPrevFrame) {
        // only one frame is needed and is already present
        animator->blend_lerp = 1.0f;
        animator->next_frame_state_index = existingNextFrame;
        return;
    }

    if (existingNextFrame == 1) {
        animator->blend_lerp = lerpValue;
    } else {
        animator->blend_lerp = 1.0f - lerpValue;
    }
}

void animator_update(struct animator* animator, struct armature* armature, float delta_time) {
    struct animation_clip* current_clip = animator->current_clip;

    if (!current_clip) {
        return;
    }

    animator_read_transform(animator, armature->pose);

    if (animator->done) {
        animator->current_clip = NULL;
        return;
    }

    animator_step(animator, delta_time);
    armature->image_frame_0 = animator->image_frame_0;
    armature->image_frame_1 = animator->image_frame_1;
    armature->prim_color = animator->prim_color;
    armature->env_color = animator->env_color;
}

void animator_run_clip(struct animator* animator, struct animation_clip* clip, float start_time, bool loop) {
    animator->current_clip = clip;

    if (!clip) {
        return;
    }

    if (animator->next_frame_state_index != -1) {
        animator->next_frame_state_index ^= 1;
    }
    
    animator->bone_state_frames[0] = -1;
    animator->bone_state_frames[1] = -1;

    animator->blend_lerp = 1.0f;

    animator->current_time = start_time;
    animator->loop = loop;
    animator->done = 0;
    animator->events.all = 0;

    animator_step(animator, 0.0f);
}

int animator_is_running(struct animator* animator) {
    return animator->current_clip != NULL;
}

bool animator_is_running_clip(struct animator* animator, struct animation_clip* clip) {
    return clip != NULL && animator->current_clip == clip;
}

float animator_get_time(struct animator* animator) {
    return animator->current_time;
}