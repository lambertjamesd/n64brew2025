#include "tmesh.h"

#include "../resource/material_cache.h"
#include "../resource/sprite_cache.h"

// T3MS
#define EXPECTED_HEADER 0x54334D53

enum TMeshCommandType {
    TMESH_COMMAND_VERTICES,
    TMESH_COMMAND_TRIANGLES,
    TMESH_COMMAND_MATERIAL,
    TMESH_COMMAND_BONE,
};

void tmesh_unpack_pos(struct Vector3* pos, FILE* file) {
    int16_t read;
    fread(&read, 2, 1, file);
    pos->x = read;
    fread(&read, 2, 1, file);
    pos->y = read;
    fread(&read, 2, 1, file);
    pos->z = read;
}

void tmesh_unpack_rot(struct Quaternion* rot, FILE* file) {
    int16_t read;
    fread(&read, 2, 1, file);
    rot->x = read * (1.0f / 32767.0f);
    fread(&read, 2, 1, file);
    rot->y = read * (1.0f / 32767.0f);
    fread(&read, 2, 1, file);
    rot->z = read * (1.0f / 32767.0f);
    rot->w = sqrtf(1.0f - rot->x * rot->x - rot->y * rot->y - rot->z * rot->z);
}

void tmesh_load_attachment(struct armature_attatchment* attatchment, FILE* file) {
    uint8_t str_len;
    fread(&str_len, 1, 1, file);

    attatchment->name = malloc(str_len + 1);
    fread(attatchment->name, 1, str_len, file);
    attatchment->name[str_len] = '\0';

    fread(&attatchment->bone_index, 2, 1, file);

    struct Transform transform;
    tmesh_unpack_pos(&transform.position, file);
    tmesh_unpack_rot(&transform.rotation, file);
    int16_t scale_packed;
    fread(&scale_packed, 2, 1, file);
    transform.scale.x = scale_packed * (1.0f / 256.0f);
    transform.scale.y = transform.scale.x;
    transform.scale.z = transform.scale.x;

    T3DMat4 mtx;
    transformToMatrix(&transform, mtx.m);
    t3d_mat4_to_fixed(UncachedAddr(&attatchment->local_transform), &mtx);
    data_cache_hit_writeback_invalidate(&attatchment->local_transform, sizeof(T3DMat4FP));
}

void tmesh_load_armature(struct tmesh* tmesh, FILE* file) {
    uint16_t bone_count;
    fread(&bone_count, 2, 1, file);

    armature_definition_init(&tmesh->armature, bone_count);

    if (!bone_count) {
        return;
    }

    fread(tmesh->armature.parent_linkage, 1, bone_count, file);
    fread(tmesh->armature.default_pose, sizeof(struct armature_packed_transform), bone_count, file);

    fread(&tmesh->armature.image_frames_0, 1, 1, file);
    fread(&tmesh->armature.image_frames_1, 1, 1, file);

    int total_images = tmesh->armature.image_frames_0 + tmesh->armature.image_frames_1;

    if (total_images) {
        tmesh->armature.frames = malloc(sizeof(sprite_t*) * total_images);
        for (int i = 0; i < total_images; i += 1) {
            uint8_t filename_len;
            fread(&filename_len, 1, 1, file);
            char filename[filename_len + 1];
            fread(filename, filename_len, 1, file);
            filename[filename_len] = '\0';
            tmesh->armature.frames[i] = sprite_cache_load(filename);
        }
    } else {
        tmesh->armature.frames = NULL;
    }

    fread(&tmesh->armature.flags, sizeof(uint16_t), 1, file);
}

void tmesh_load(struct tmesh* tmesh, FILE* file) {
    int header;

    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    // load material
    tmesh->material = material_cache_load_from_file(file);

    // load vertices

    fread(&tmesh->vertex_count, sizeof(uint16_t), 1, file);
    tmesh->vertices = malloc(sizeof(T3DVertPacked) * tmesh->vertex_count);
    fread(&tmesh->vertices[0], sizeof(T3DVertPacked), tmesh->vertex_count, file);
    data_cache_hit_writeback(&tmesh->vertices[0], sizeof(T3DVertPacked) * tmesh->vertex_count);

    // load material transitions

    uint16_t transition_count;
    fread(&transition_count, sizeof(uint16_t), 1, file);
    tmesh->material_transition_count = transition_count;

    if (transition_count) {
        tmesh->transition_materials = malloc(sizeof(struct material) * transition_count);

        for (int i = 0; i < transition_count; i += 1) {
            material_load(&tmesh->transition_materials[i], file);
        }
    } else {
        tmesh->transition_materials = NULL;
    }

    tmesh_load_armature(tmesh, file);

    // load attatchments
    fread(&tmesh->attatchment_count, 2, 1, file);

    if (tmesh->attatchment_count) {
        tmesh->attatchments = malloc(sizeof(struct armature_attatchment) * tmesh->attatchment_count);

        for (int i = 0; i < tmesh->attatchment_count; i += 1) {
            struct armature_attatchment* attatchment = &tmesh->attatchments[i];
            tmesh_load_attachment(attatchment, file);
        }
    } else {
        tmesh->attatchments = NULL;
    }

    fread(&tmesh->light_source, 1, 1, file);

    // load mesh draw commands
    
    uint16_t command_count;
    fread(&command_count, sizeof(uint16_t), 1, file);

    bool has_bone = false;

    T3DMat4FP* armature = t3d_segment_placeholder(T3D_SEGMENT_SKELETON);

    rspq_block_begin();

    for (uint16_t i = 0; i < command_count; i += 1) {
        uint8_t command;
        fread(&command, sizeof(uint8_t), 1, file);

        switch (command) 
        {
            case TMESH_COMMAND_VERTICES:
                {
                    uint8_t offset;
                    uint8_t count;
                    uint16_t vertex_source;
                    fread(&offset, sizeof(uint8_t), 1, file);
                    fread(&count, sizeof(uint8_t), 1, file);
                    fread(&vertex_source, sizeof(uint16_t), 1, file);
                    t3d_vert_load(&tmesh->vertices[vertex_source], offset, count);
                    break;
                }
            case TMESH_COMMAND_TRIANGLES:
            {
                uint8_t triangle_count;
                fread(&triangle_count, sizeof(uint8_t), 1, file);

                for (int i = 0 ; i < triangle_count; i += 1) {
                    uint8_t indices[3];
                    fread(&indices[0], sizeof(uint8_t), 3, file);
                    t3d_tri_draw(indices[0], indices[1], indices[2]);
                }
                t3d_tri_sync();
                break;
            }
            case TMESH_COMMAND_MATERIAL:
            {
                uint16_t material_index;
                fread(&material_index, sizeof(uint16_t), 1, file);
                assert(material_index < tmesh->material_transition_count);
                rspq_block_run(tmesh->transition_materials[material_index].block);
                break;
            }
            case TMESH_COMMAND_BONE:
            {
                uint16_t bone_index;
                fread(&bone_index, sizeof(uint16_t), 1, file);
                if (bone_index == 0xFFFF) {
                    if (has_bone) {
                        t3d_matrix_pop(1);
                        has_bone = false;
                    }
                } else if (has_bone) {
                    t3d_matrix_set(&armature[bone_index], true);
                } else {
                    t3d_matrix_push(&armature[bone_index]);
                    has_bone = true;
                }
                break;
            }
            default:
                assert(false);
        }
    }

    if (has_bone) {
        t3d_matrix_pop(1);
    }

    tmesh->block = rspq_block_end();
}

void tmesh_release(struct tmesh* tmesh) {
    rspq_block_free(tmesh->block);
    free(tmesh->vertices);

    if (tmesh->material) {
        material_cache_release(tmesh->material);
    }

    if (tmesh->transition_materials) {
        for (int i = 0; i < tmesh->material_transition_count; i += 1) {
            material_release(&tmesh->transition_materials[i]);
        }

        free(tmesh->transition_materials);
    }

    if (tmesh->attatchments) {
        for (int i = 0; i < tmesh->attatchment_count; i += 1) {
            free(tmesh->attatchments[i].name);
        }
        free(tmesh->attatchments);
    }

    int total_frames = tmesh->armature.image_frames_0 + tmesh->armature.image_frames_1;

    for (int i = 0; i < total_frames; i += 1) {
        sprite_cache_release(tmesh->armature.frames[i]);
    }

    free(tmesh->armature.frames);

    armature_definition_destroy(&tmesh->armature);
}