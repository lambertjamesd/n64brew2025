#include "render_batch.h"

#include "../util/sort.h"
#include "../time/time.h"
#include "../particles/static_particles.h"
#include "defs.h"

void render_batch_init(struct render_batch* batch, struct Transform* camera_transform, struct frame_memory_pool* pool) {
    batch->element_count = 0;
    batch->pool = pool;

    transformToMatrix(camera_transform, batch->camera_matrix);

    batch->rotation_2d.x = batch->camera_matrix[0][0];
    batch->rotation_2d.y = batch->camera_matrix[0][2];
    vector2Normalize(&batch->rotation_2d, &batch->rotation_2d);
}

struct render_batch_element* render_batch_add(struct render_batch* batch) {
    if (batch->element_count >= RENDER_BATCH_MAX_SIZE) {
        return NULL;
    }

    struct render_batch_element* result = &batch->elements[batch->element_count];
    ++batch->element_count;

    result->material = NULL;
    result->type = RENDER_BATCH_MESH;
    result->mesh.block = 0;
    result->mesh.attrs = NULL;
    result->light_source = 0;

    return result;
}

T3DMat4FP* render_batch_build_pose(T3DMat4* pose, int bone_count) {
    if (!pose) {
        return NULL;
    }

    T3DMat4* end = pose + bone_count;

    for(T3DMat4* curr = pose; curr < end; curr += 1) {
        T3DMat4FP tmp;
        t3d_mat4_to_fixed(&tmp, curr);
        *((T3DMat4FP*)curr) = tmp;
    }

    data_cache_hit_writeback_invalidate(pose, sizeof(T3DMat4) * bone_count);

    return (T3DMat4FP*)pose;
}

void render_batch_relative_mtx(struct render_batch* batch, mat4x4 into) {
    into[3][0] -= batch->camera_matrix[3][0] * WORLD_SCALE;
    into[3][1] -= batch->camera_matrix[3][1] * WORLD_SCALE;
    into[3][2] -= batch->camera_matrix[3][2] * WORLD_SCALE;
}

struct render_batch_element* render_batch_add_tmesh(
    struct render_batch* batch, 
    struct tmesh* mesh, 
    T3DMat4FP* transform,
    struct armature* armature, 
    struct tmesh** attachments,
    struct element_attr* additional_attrs
) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return NULL;
    }

    element->mesh.block = mesh->block;
    element->material = mesh->material;
    element->light_source = mesh->light_source;
    int attr_count = 0;
    element->mesh.attrs = NULL;

    if (additional_attrs) {
        struct element_attr* curr = additional_attrs;
        while (curr->type != ELEMENT_ATTR_NONE) {
            ++attr_count;
            ++curr;
        }
    }

    if (transform) {
        ++attr_count;
    }

    if (armature) {
        if (armature->bone_count) {
            ++attr_count;
        }

        if (armature->image_frame_0 != NO_IMAGE_FRAME) {
            ++attr_count;
        }

        if (armature->has_prim_color) {
            attr_count += 1;
        }

        if (armature->has_env_color) {
            attr_count += 1;
        }
    }

    if (attr_count) {
        element->mesh.attrs = frame_malloc(batch->pool, sizeof(struct element_attr) * (attr_count + 1));
    
        if (!element->mesh.attrs) {
            element->mesh.attrs = 0;
            return NULL;
        }
    } else {
        return element;
    }

    struct element_attr* attr = element->mesh.attrs;

    while (additional_attrs && additional_attrs->type != ELEMENT_ATTR_NONE) {
        *attr = *additional_attrs;
        ++attr;
        ++additional_attrs;
    }

    if (transform) {
        attr->type = ELEMENT_ATTR_TRANSFORM;
        attr->transform = transform;
        ++attr;
    }

    if (armature) {
        if (armature->bone_count) {
            T3DMat4* pose = armature_build_pose(armature, batch->pool);
    
            T3DMat4FP* pose_fp = render_batch_build_pose(pose, armature->bone_count);
            
            attr->type = ELEMENT_ATTR_POSE;
            attr->offset = 0;
            attr->pose.pose = pose_fp;
    
            if (attachments && mesh->attatchments) {
                for (int i = 0; i < mesh->attatchment_count; i += 1) {
                    if (!attachments[i]) {
                        continue;
                    }
    
                    T3DMat4FP** matrices = frame_malloc(batch->pool, sizeof(T3DMat4FP*) * 3);
    
                    if (!matrices) {
                        continue;
                    }
    
                    int matrix_index = 0;                
                    
                    struct armature_attatchment* linkage = &mesh->attatchments[i];
                    
                    matrices[matrix_index++] = transform;
                    matrices[matrix_index++] = &pose_fp[linkage->bone_index];
                    matrices[matrix_index++] = &linkage->local_transform;

                    struct element_attr attachment_attrs[2];
                    attachment_attrs[0].type = ELEMENT_ATTR_TRANSFORM_LIST;
                    attachment_attrs[0].offset = 2;
                    attachment_attrs[0].transform_list = matrices;
                    attachment_attrs[1].type = ELEMENT_ATTR_NONE;
    
                    render_batch_add_tmesh(batch, attachments[i], NULL, NULL, NULL, attachment_attrs);
                }
            }
            ++attr;
        }

        if (armature->image_frame_0 != NO_IMAGE_FRAME && armature->image_frame_0 < armature->definition->image_frames_0) {
            attr->type = ELEMENT_ATTR_IMAGE;
            attr->offset = 0;
            attr->image.sprite = armature->definition->frames[armature->image_frame_0];
            ++attr;
        }

        if (armature->has_prim_color) {
            attr->type = ELEMENT_ATTR_PRIM_COLOR;
            attr->color = armature->prim_color;
            ++attr;
        }

        if (armature->has_env_color) {
            attr->type = ELEMENT_ATTR_ENV_COLOR;
            attr->color = armature->env_color;
            ++attr;
        }
    }
    
    if (attr) {
        attr->type = ELEMENT_ATTR_NONE;
    }

    return element;
}

void render_batch_add_callback(struct render_batch* batch, struct material* material, RenderCallback callback, void* data) {
    struct render_batch_element* element = render_batch_add(batch);

    if (!element) {
        return;
    }

    element->type = RENDER_BATCH_CALLBACK;
    element->material = material;
    element->callback.callback = callback;
    element->callback.data = data;
}

struct render_batch_element* render_batch_add_particles(
    struct render_batch* batch, 
    struct material* material, 
    render_batch_particles_t* particles, 
    T3DMat4FP* mtx
) {
    struct render_batch_element* result = render_batch_add(batch);

    result->type = RENDER_BATCH_PARTICLES;
    result->material = material;
    result->particles.particles = particles;
    result->particles.transform = mtx;

    return result;
}


struct render_batch_element* render_batch_add_dynamic_particles(
    struct render_batch* batch, 
    struct material* material, 
    int count, 
    const struct render_batch_particle_size* size,
    T3DMat4FP* mtx
) {
    int aligned_count = (count + 1) & ~1;
    render_batch_particles_t* particles = frame_malloc(batch->pool, ALIGN_16(sizeof(render_batch_particles_t)) + sizeof(TPXParticle) * (aligned_count >> 1));

    TPXParticle* tpx_particles = (TPXParticle*)(ALIGN_16((int)(particles + 1)));
    particles->particles = tpx_particles;
    particles->particle_count = aligned_count;
    particles->particle_size = size->particle_size;
    particles->particle_scale_width = size->particle_scale_width;
    particles->particle_scale_height = size->particle_scale_height;

    if (aligned_count != count) {
        TPXParticle* last = &tpx_particles[(aligned_count >> 1) - 1];
        last->sizeB = 0;
    }

    return render_batch_add_particles(batch, material, particles, mtx);
}

mat4x4* render_batch_get_transform(struct render_batch* batch) {
    return frame_malloc(batch->pool, sizeof(mat4x4));
}

T3DMat4FP* render_batch_get_transformfp(struct render_batch* batch) {
    return UncachedAddr(frame_malloc(batch->pool, sizeof(T3DMat4FP)));
}

T3DMat4FP* render_batch_transformfp_from_sa(struct render_batch* batch, struct TransformSingleAxis* transform) {
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return NULL;
    }

    mat4x4 mtx;
    transformSAToMatrix(transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    return mtxfp;
}

T3DMat4FP* render_batch_transformfp_from_full(struct render_batch* batch, struct Transform* transform) {
    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return NULL;
    }

    mat4x4 mtx;
    transformToWorldMatrix(transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    return mtxfp;
}

int render_batch_compare_element(struct render_batch* batch, uint16_t a_index, uint16_t b_index) {
    struct render_batch_element* a = &batch->elements[a_index];
    struct render_batch_element* b = &batch->elements[b_index];

    if (a == b) {
        return 0;
    }

    if (a->material->sort_priority != b->material->sort_priority) {
        return a->material->sort_priority - b->material->sort_priority;
    }

    if (a->material != b->material) {
        return (int)a->material - (int)b->material;
    }

    return a->type - b->type;
}

void render_batch_check_texture_scroll(int tile, struct material_tex* tex) {
    if (!tex->texture_enabled || (!tex->scroll_x && !tex->scroll_y)) {
        return;
    }

    int w = tex->width << 2;
    int h = tex->height << 2;

    int x_offset = (int)(game_time * tex->scroll_x * w) % w;
    int y_offset = (int)(game_time * tex->scroll_y * h) % h;

    if (x_offset < 0) {
        x_offset += w;
    }

    if (y_offset < 0) {
        y_offset += h;
    }

    rdpq_set_tile_size_fx(
        tile, 
        x_offset + tex->s0, y_offset + tex->t0, 
        x_offset + tex->s1, y_offset + tex->t1
    );
}

static bool element_type_2d[] = {
    [RENDER_BATCH_MESH] = false,
    [RENDER_BATCH_PARTICLES] = true,
    [RENDER_BATCH_CALLBACK] = false,
};

static uint8_t white_color[4] = {0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t black_color[4] = {0x00, 0x00, 0x00, 0xFF};

void render_batch_setup_light(struct render_batch* batch, enum light_source light_source) {
    switch (light_source) {
        case LIGHT_SOURCE_NONE:
            t3d_light_set_ambient(white_color);
            t3d_light_set_count(0);
            break;
        case LIGHT_SOURCE_CAMERA: {
            T3DVec3 dir = {{
                .x = batch->camera_matrix[2][0],
                .y = batch->camera_matrix[2][1],
                .z = batch->camera_matrix[2][2],
            }};
            t3d_light_set_ambient(black_color);
            t3d_light_set_directional(0, white_color, &dir);
            t3d_light_set_count(1);
            break;
        }
    }
}

void render_batch_finish(struct render_batch* batch, mat4x4 view_proj_matrix, T3DViewport* viewport) {
    uint16_t order[RENDER_BATCH_MAX_SIZE];
    uint16_t order_tmp[RENDER_BATCH_MAX_SIZE];

    for (int i = 0; i < batch->element_count; ++i) {
        order[i] = i;
    }

    // used to scale billboard sprites
    float scale_x = sqrtf(
        view_proj_matrix[0][0] * view_proj_matrix[0][0] + 
        view_proj_matrix[0][1] * view_proj_matrix[0][1] +
        view_proj_matrix[0][2] * view_proj_matrix[0][2]
    ) * 0.5f * 4;

    float scale_y = sqrtf(
        view_proj_matrix[1][0] * view_proj_matrix[1][0] + 
        view_proj_matrix[1][1] * view_proj_matrix[1][1] +
        view_proj_matrix[1][2] * view_proj_matrix[1][2]
    ) * 0.5f * 4;

    sort_indices(order, batch->element_count, batch, (sort_compare)render_batch_compare_element);

    struct material* current_mat = 0;

    rdpq_set_mode_standard();
    rdpq_mode_persp(true);
    rdpq_mode_zbuf(true, true);
	rdpq_mode_dithering(DITHER_SQUARE_INVSQUARE);
    t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_TEXTURED);

    bool is_sprite_mode = false;
    bool z_write = true;
    bool z_read = true;
    enum light_source light_source = LIGHT_SOURCE_NONE;
    render_batch_setup_light(batch, light_source);

    T3DMat4FP* default_mtx = render_batch_get_transformfp(batch);

    if (default_mtx) {
        mat4x4 scaleMtx;
        matrixFromScale(scaleMtx, STATIC_WORLD_SCALE);
        struct Vector3 camera_neg_pos = {
            batch->camera_matrix[3][0],
            batch->camera_matrix[3][1],
            batch->camera_matrix[3][2],
        };
        matrixApplyScaledPos(scaleMtx, &camera_neg_pos, -WORLD_SCALE);

        if (fabsf(scaleMtx[3][0]) > 0x7fff || fabsf(scaleMtx[3][1]) > 0x7fff || fabsf(scaleMtx[3][2]) > 0x7fff) {
            default_mtx = NULL;
        } else {
            t3d_mat4_to_fixed_3x4(default_mtx, (T3DMat4*)scaleMtx);
        }
    }

    for (int i = 0; i < batch->element_count; ++i) {
        int index = order[i];
        struct render_batch_element* element = &batch->elements[index];

        if (current_mat != element->material) {
            rdpq_sync_pipe();
            if (element->material->block) {
                rspq_block_run(element->material->block);
            }

            render_batch_check_texture_scroll(TILE0, &element->material->tex0);
            render_batch_check_texture_scroll(TILE1, &element->material->tex1);

            bool need_z_write = (element->material->flags & MATERIAL_FLAGS_Z_WRITE) != 0;
            bool need_z_read = (element->material->flags & MATERIAL_FLAGS_Z_READ) != 0;

            if (need_z_write != z_write || need_z_read != z_read) {
                rdpq_mode_zbuf(need_z_read, need_z_write);
                z_write = need_z_write;
                z_read = need_z_read;
            }

            current_mat = element->material;
        }

        if (light_source != element->light_source) {
            render_batch_setup_light(batch, element->light_source);
            light_source = element->light_source;
        }

        bool should_sprite_mode = element_type_2d[element->type];

        if (should_sprite_mode != is_sprite_mode) {
            rdpq_sync_pipe();
            if (should_sprite_mode) {
                static_particles_start();
            } else {
                static_particles_end();
            }

            is_sprite_mode = should_sprite_mode;
        }

        if (element->type == RENDER_BATCH_MESH) {
            if (!element->mesh.block) {
                continue;
            }

            int transform_count = 0;

            for (struct element_attr* attr = element->mesh.attrs; attr && attr->type != ELEMENT_ATTR_NONE; ++attr) {
                switch (attr->type) {
                    case ELEMENT_ATTR_POSE:
                        t3d_segment_set(T3D_SEGMENT_SKELETON, attr->pose.pose);
                        break;
                    case ELEMENT_ATTR_IMAGE:
                        rdpq_set_lookup_address(attr->offset+1, (void*)PhysicalAddr(attr->image.sprite->data));
                        break;
                    case ELEMENT_ATTR_PRIM_COLOR:
                        rdpq_set_prim_color(attr->color);
                        break;
                    case ELEMENT_ATTR_ENV_COLOR:
                        rdpq_set_env_color(attr->color);
                        break;
                    case ELEMENT_ATTR_TRANSFORM:
                        t3d_matrix_push(attr->transform);
                        transform_count = 1;
                        break;
                    case ELEMENT_ATTR_SCROLL:
                        if (current_mat) {
                            struct material_tex* tex = &current_mat->tex0;
                            rdpq_set_tile_size_fx(
                                TILE0, 
                                attr->scroll.x + tex->s0, attr->scroll.y + tex->t0, 
                                attr->scroll.x + tex->s1, attr->scroll.y + tex->t1
                            );
                        }
                        break;
                    case ELEMENT_ATTR_TRANSFORM_LIST: {
                        for (int mtx_index = 0; mtx_index < attr->offset; mtx_index += 1) {
                            t3d_matrix_push(attr->transform_list[mtx_index]);
                        }
                        transform_count = attr->offset;
                        break;
                    }
                }
            }

            if (transform_count == 0 && default_mtx) {
                t3d_matrix_push(default_mtx);
                transform_count = 1;
            }

            rspq_block_run(element->mesh.block);

            if (transform_count) {
                t3d_matrix_pop(transform_count);
            }
        } else if (element->type == RENDER_BATCH_PARTICLES) {
            static_particles_render(element->particles.particles, element->particles.transform, current_mat != NULL && current_mat->tex0.sprite != NULL);
        } else if (element->type == RENDER_BATCH_CALLBACK) {
            element->callback.callback(element->callback.data, batch);
        }
    }
    rdpq_sync_pipe();
}