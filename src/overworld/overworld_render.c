#include "overworld_render.h"

#include <stdlib.h>
#include "../math/mathf.h"
#include "overworld_private.h"
#include "../render/defs.h"

static int edge_deltas[] = {0x1, 0x2, 0x4};

int overworld_find_next_edge(struct Vector2 transformed_points[8], int current_index, int prev_index) {
    int result = -1;

    for (int i = 0; i < 3; i += 1) {
        int next_index = current_index ^ edge_deltas[i];

        if (next_index == prev_index) {
            continue;
        }

        if (vector2DistSqr(&transformed_points[current_index], &transformed_points[next_index]) < 0.00001f) {
            continue;
        }

        if (result == -1) {
            result = next_index;
            continue;
        }

        struct Vector2 edge_a;
        struct Vector2 edge_b;
        vector2Sub(&transformed_points[next_index], &transformed_points[current_index], &edge_a);
        vector2Sub(&transformed_points[result], &transformed_points[current_index], &edge_b);

        if (vector2Cross(&edge_a, &edge_b) > 0.00001f) {
            result = next_index;
        }
    }

    return result;
}

int overworld_create_top_view(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct Vector2* loop) {
    mat4x4 view_inv;

    if (!matrixInv(view_proj_matrix, view_inv)) {
        return 0;
    }

    struct Vector2 transformed_points[8];

    int current_index = 0;

    for (int i = 0; i < 8; i += 1) {
        struct Vector3 point = {
            (i & 0x1) ? 1.0f : -1.0f,
            (i & 0x2) ? 1.0f : -1.0f,
            (i & 0x4) ? 1.0f : -1.0f,
        };

        struct Vector4 transformed_point;
        matrixVec3Mul(view_inv, &point, &transformed_point);

        float inv_w = 1.0f / (transformed_point.w * WORLD_SCALE);
        
        transformed_points[i].x = (transformed_point.x * inv_w + camera_position->x - overworld->min.x) * overworld->inv_tile_size;
        transformed_points[i].y = (transformed_point.z * inv_w + camera_position->z - overworld->min.y) * overworld->inv_tile_size;

        if (transformed_points[i].y < transformed_points[current_index].y) {
            current_index = i;
        }
    }

    int result = 0;

    loop[result++] = transformed_points[current_index];
    int prev_index = current_index;
    int start_index = current_index;
    current_index = overworld_find_next_edge(transformed_points, current_index, current_index);
    loop[result++] = transformed_points[current_index];

    while (result < 8) {
        int next_index = overworld_find_next_edge(transformed_points, current_index, prev_index);

        if (next_index == start_index) {
            break;
        }

        prev_index = current_index;
        current_index = next_index;

        loop[result++] = transformed_points[next_index];
    }

    return result;
}

#define PREV_INDEX(state, current) ((current) == 0 ? state->loop_count - 1 : (current) - 1)
#define NEXT_INDEX(state, current) ((current) + 1 == state->loop_count ? 0 : (current) + 1)

float overworld_interpolate_x(struct Vector2* p0, struct Vector2* p1, float midy) {
    float t = (midy - p0->y) / (p1->y - p0->y);
    return (p1->x - p0->x) * t + p0->x;
}

struct overworld_tile_slice overworld_step(struct overworld* overworld, struct overworld_step_state* state) {
    float min_x = state->min_x;
    float max_x = state->max_x;

    float next_y = ceilf(state->current_y);

    if (next_y == state->current_y) {
        next_y += 1.0f;
    }

    bool result = true;

    for (int i = 0; i < state->loop_count; i += 1) {
        int prev_index = PREV_INDEX(state, state->left);

        struct Vector2* current_position = &state->loop[state->left];
        struct Vector2* prev_pos = &state->loop[prev_index];

        if (prev_pos->y < current_position->y) {
            result = false;
            break;
        }

        if (prev_pos->y < next_y) {
            min_x = minf(min_x, prev_pos->x);
            state->left = prev_index;
        } else {
            state->min_x = overworld_interpolate_x(current_position, prev_pos, next_y);
            min_x = minf(min_x, state->min_x);
            break;
        }
    }

    for (int i = 0; i < state->loop_count; i += 1) {
        int next_index = NEXT_INDEX(state, state->right);

        struct Vector2* current_position = &state->loop[state->right];
        struct Vector2* next_pos = &state->loop[next_index];

        if (next_pos->y < current_position->y) {
            result = false;
            break;
        }

        if (next_pos->y < next_y) {
            max_x = maxf(max_x, next_pos->x);
            state->right = next_index;
        } else {
            state->max_x = overworld_interpolate_x(current_position, next_pos, next_y);
            max_x = maxf(max_x, state->max_x);
            break;
        }
    }

    int y_int = (int)floorf(state->current_y);
    state->current_y = next_y;

    if (y_int < 0 || y_int >= overworld->tile_y) {
        return (struct overworld_tile_slice){
            .has_more = result && y_int < overworld->tile_y
        };
    }

    int min_int = (int)floorf(min_x);

    if (min_int < 0) {
        min_int = 0;
    }

    int max_int = (int)ceilf(max_x);

    if (max_int >= overworld->tile_x) {
        max_int = overworld->tile_x - 1;
    }

    return (struct overworld_tile_slice){
        .min_x = min_int,
        .max_x = max_int,
        .y = y_int,
        .has_more = result,
    };
}

struct overworld_lod0_sort_entry {
    uint32_t priority;
    struct tmesh* mesh;
};

int overworld_entry_sort(const void* a, const void* b) {
    const struct overworld_lod0_sort_entry* a_entry = (const struct overworld_lod0_sort_entry*)a;
    const struct overworld_lod0_sort_entry* b_entry = (const struct overworld_lod0_sort_entry*)b;
    return (int)(b_entry->priority - a_entry->priority);
}

int overworld_lod_0_direction_idnex(int dx, int dy) {
    if (abs(dx) > abs(dy)) {
        return dx > 0 ? 1 : 0;
    }

    return dy > 0 ? 3 : 2;
}

void overworld_render_lod_0_entries(struct overworld_lod0* lod0, int camera_x, int camera_z) {
    struct overworld_lod0_sort_entry order[lod0->entry_count];

    struct overworld_lod0_entry* end = lod0->entries + lod0->entry_count;
    struct overworld_lod0_sort_entry* entry = order;
    for (struct overworld_lod0_entry* curr = lod0->entries; curr < end; curr += 1, entry += 1) {
        int dx = (int)curr->x - camera_x;
        int dy = (int)curr->z - camera_z;
        entry->priority = ((dx * dx + dy * dy) >> 2) - ((uint32_t)(curr->priority) << 30);
        
        entry->mesh = &curr->meshes[overworld_lod_0_direction_idnex(dx, dy)];
    }

    qsort(order, lod0->entry_count, sizeof(struct overworld_lod0_sort_entry), overworld_entry_sort);

    struct material* mat = NULL;

    for (int i = 0; i < lod0->entry_count; i += 1) {
        struct tmesh* mesh = order[i].mesh;

        if (mat != mesh->material) {
            material_apply(mesh->material);
            mat = mesh->material;
        }
        rspq_block_run(mesh->block);
    }
}

void overworld_render_lod_0(struct overworld* overworld, struct Camera* camera, T3DViewport* prev_viewport, struct frame_memory_pool* pool) {
    T3DViewport* new_viewport = frame_malloc(pool, sizeof(T3DViewport));
    *new_viewport = t3d_viewport_create();
    
    float tan_fov = tanf(camera->fov * (0.5f * 3.14159f / 180.0f));
    float aspect_ratio = (float)prev_viewport->size[0] / (float)prev_viewport->size[1];

    float lod_scale = 1.0f / overworld->tile_x;

    float near = (camera->far - 5.0f) * lod_scale * WORLD_SCALE;
    float far = overworld->tile_size * (1.4f * WORLD_SCALE);
    
    matrixPerspective(
        new_viewport->matProj.m, 
        -aspect_ratio * tan_fov * near,
        aspect_ratio * tan_fov * near,
        tan_fov * near,
        -tan_fov * near,
        near,
        far
    );
    t3d_viewport_set_w_normalize(new_viewport, camera->near * WORLD_SCALE, camera->far * WORLD_SCALE);

    struct Transform inverse;
    transformInvert(&camera->transform, &inverse);
    inverse.position = gZeroVec;
    transformToMatrix(&inverse, new_viewport->matCamera.m);
    new_viewport->_isCamProjDirty = true;

    t3d_viewport_attach(new_viewport);

    T3DMat4 mtx;
    t3d_mat4_identity(&mtx);
    t3d_mat4_translate(
        &mtx, 
        -camera->transform.position.x * lod_scale * WORLD_SCALE,
        -camera->transform.position.y * lod_scale * WORLD_SCALE,
        -camera->transform.position.z * lod_scale * WORLD_SCALE
    );
    mtx.m[0][0] = STATIC_WORLD_SCALE;
    mtx.m[1][1] = STATIC_WORLD_SCALE;
    mtx.m[2][2] = STATIC_WORLD_SCALE;

    int camera_x = -(int)mtx.m[3][0];
    int camera_z = -(int)mtx.m[3][2];

    rdpq_sync_pipe();
    rdpq_mode_zbuf(false, false);

    T3DMat4FP* mtx_fp = UncachedAddr(frame_malloc(pool, sizeof(T3DMat4FP)));
    t3d_mat4_to_fixed_3x4(mtx_fp, &mtx);
    t3d_matrix_push(mtx_fp);
    overworld_render_lod_0_entries(&overworld->lod0, camera_x, camera_z);
    t3d_matrix_pop(1);

    rdpq_sync_pipe();
    rdpq_mode_zbuf(true, true);
} 

void overworld_render_tile(struct overworld* overworld, struct Camera* camera, struct frame_memory_pool* pool, int x, int z) {
    struct overworld_tile_render_block* block = &overworld->render_blocks[x & 0x3][z & 0x3];
    struct Vector3* camera_position = &camera->transform.position;

    if (!block->render_blocks || block->x != x || block->z != z) {
        overworld->load_next.x = x;
        overworld->load_next.y = z;
        return;
    }

    int min_y = (int)floorf((camera_position->y - block->starting_y - camera->far) * overworld->inv_tile_size);
    int max_y = (int)ceilf((camera_position->y - block->starting_y + camera->far) * overworld->inv_tile_size);

    if (min_y < 0) {
        min_y = 0;
    }

    if (max_y > block->y_height) {
        max_y = block->y_height;
    }

    T3DMat4 mtx;
    t3d_mat4_identity(&mtx);

    t3d_mat4_translate(
        &mtx, 
        (x * overworld->tile_size + overworld->min.x - camera_position->x) * WORLD_SCALE,
        (min_y * overworld->tile_size + block->starting_y - camera_position->y) * WORLD_SCALE,
        (z * overworld->tile_size + overworld->min.y - camera_position->z) * WORLD_SCALE
    );

    mtx.m[0][0] = STATIC_WORLD_SCALE;
    mtx.m[1][1] = STATIC_WORLD_SCALE;
    mtx.m[2][2] = STATIC_WORLD_SCALE;

    for (int y = min_y; y < max_y; y += 1) {
        T3DMat4FP* tile_position = frame_malloc(pool, sizeof(T3DMat4FP));
    
        if (!tile_position) {
            return;
        }
    
        tile_position = UncachedAddr(tile_position);
    
        t3d_mat4_to_fixed_3x4(tile_position, &mtx);
    
        t3d_matrix_push(tile_position);
        rspq_block_run(block->render_blocks[y]);
        t3d_matrix_pop(1);

        mtx.m[3][1] += overworld->tile_size * WORLD_SCALE;
    }

    if (block->tile->static_particle_count) {
        static_particles_start();
        static_particles_render_instances(block->tile->static_particles, block->tile->static_particle_count, pool, camera_position);
        static_particles_end();
    }
}

void overworld_render(struct overworld* overworld, mat4x4 view_proj_matrix, struct Camera* camera, T3DViewport* viewport, struct frame_memory_pool* pool) {
    struct overworld_step_state state;
    struct Vector3* camera_position = &camera->transform.position;
    state.loop_count = overworld_create_top_view(overworld, view_proj_matrix, camera_position, state.loop);
    state.left = 0;
    state.right = 0;
    state.current_y = state.loop[0].y;
    state.min_x = state.loop[0].x;
    state.max_x = state.loop[0].x;

    overworld_render_lod_0(overworld, camera, viewport, pool);

    t3d_viewport_attach(viewport);

    if (!state.loop_count) {
        return;
    }

    for (int i = 0; i < 4; i += 1) {
        struct overworld_tile_slice next = overworld_step(overworld, &state);

        for (int x = next.min_x; x < next.max_x; x += 1) {
            overworld_render_tile(overworld, camera, pool, x, next.y);
        }
        
        if (!next.has_more) {
            break;
        }
    }
}