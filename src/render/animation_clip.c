#include "animation_clip.h"

#include <string.h>
#include <malloc.h>
#include <libdragon.h>

struct animation_set_header {
    uint16_t clip_count;
    uint16_t name_buffer_length;
    uint16_t bone_count;
};

struct animation_clip_header {
    uint16_t frame_count;
    uint16_t frames_per_second;
    uint16_t frame_size;

    uint16_t has_events: 1;
    uint16_t has_image_frames: 1;
    uint16_t has_prim_color: 1;
    uint16_t has_env_color: 1;
};

#define EXPECTED_HEADER 0x414E494D

// dma_read_raw_async
// dfs_rom_addr

struct animation_set* animation_set_load(const char* filename) {
    struct animation_set* result = malloc(sizeof(struct animation_set));

    assert(strncmp(filename, "rom:/", 5) == 0);
    filename += 5;

    int file = dfs_open(filename);

    struct animation_set_header header;

    uint32_t anim;
    dfs_read(&anim, sizeof(uint32_t), 1, file);
    assert(anim == EXPECTED_HEADER);

    dfs_read(&header, sizeof(struct animation_set_header), 1, file);

    result->clip_count = header.clip_count;
    result->bone_count = header.bone_count;

    if (result->clip_count == 0) {
        result->clips = NULL;
        return result;
    }

    struct animation_clip_header clip_headers[header.clip_count];
    dfs_read(clip_headers, sizeof(struct animation_clip_header), header.clip_count, file);

    result->clips = malloc(sizeof(struct animation_clip) * header.clip_count);

    int attibute_buffer_size = sizeof(struct animation_used_attributes) * header.bone_count * header.clip_count;
    struct animation_used_attributes* attributes_buffer = malloc(attibute_buffer_size);
    dfs_read(attributes_buffer, attibute_buffer_size, 1, file);
    
    char* text_buffer = malloc(header.name_buffer_length);
    dfs_read(text_buffer, header.name_buffer_length, 1, file);

    uint32_t start_address = dfs_rom_addr(filename);
    assert(start_address);

    uint32_t address_offset = dfs_tell(file);
    address_offset = (address_offset + 1) & ~1;

    for (int i = 0; i < header.clip_count; i += 1) {
        // align to 2 bytes
        address_offset = (address_offset + 1) & ~1;

        struct animation_clip* clip = &result->clips[i];

        clip->name = text_buffer;

        clip->bone_count = header.bone_count;
        clip->frame_count = clip_headers[i].frame_count;
        clip->frames_per_second = clip_headers[i].frames_per_second;
        clip->frame_size = clip_headers[i].frame_size;
        clip->has_events = clip_headers[i].has_events;
        clip->has_image_frames = clip_headers[i].has_image_frames;
        clip->has_prim_color = clip_headers[i].has_prim_color;
        clip->has_env_color = clip_headers[i].has_env_color;
        clip->frames_rom_address = start_address + address_offset;

        clip->used_bone_attributes = attributes_buffer;

        // advance to next string
        text_buffer += strlen(text_buffer) + 1;
        attributes_buffer += header.bone_count;
        address_offset += clip_headers[i].frame_size * clip_headers[i].frame_count;
    }

    dfs_close(file);

    return result;
}

void annotation_clip_set_free(struct animation_set* animation_set) {
    if (!animation_set) {
        return;
    }

    if (animation_set->clip_count > 0) {
        free(animation_set->clips[0].name);
        free(animation_set->clips[0].used_bone_attributes);
        free(animation_set->clips);
    }

    free(animation_set);
}

struct animation_clip* animation_set_find_clip(struct animation_set* set, const char* clip_name) {
    if (!set) {
        return NULL;
    }

    for (int i = 0; i < set->clip_count; i += 1) {
        if (strcmp(set->clips[i].name, clip_name) == 0) {
            return &set->clips[i];
        }
    }

    return NULL;
}

float animation_clip_get_duration(struct animation_clip* clip) {
    return (float)clip->frame_count / (float)clip->frames_per_second;
}