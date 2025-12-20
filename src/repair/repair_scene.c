#include "repair_scene.h"

#include "../render/defs.h"
#include "../time/time.h"
#include <malloc.h>
#include "../math/mathf.h"

// WRLD
#define EXPECTED_HEADER 0x57524C44
repair_scene_t* current_repair_scene;

static color_t basic_color = {255, 255, 255, 255};
static color_t hover_color = {20, 200, 255, 255};

#define CURSOR_SIZE 32

#define NEAR_PLANE      0.5f
#define REALLY_FAR  10000000.0f

void repair_scene_render_cursor(repair_scene_t* scene) {
    rspq_block_run(scene->assets.cursor_material.block);

    int x = (int)scene->screen_cursor.x - CURSOR_SIZE/2;
    int y = (int)scene->screen_cursor.y - CURSOR_SIZE/2;

    rdpq_set_prim_color(hover_color);
    rdpq_texture_rectangle(TILE0, x, y, x + CURSOR_SIZE, y + CURSOR_SIZE, 0, 0);
}

void repair_scene_render(repair_scene_t* scene, T3DViewport* viewport, struct frame_memory_pool* pool) {
    float tan_fov = tanf(scene->camera_fov * 0.5f);
    float aspect_ratio = (float)viewport->size[0] / (float)viewport->size[1];

    float near = 0.5f * WORLD_SCALE;
    float far = 50.0f * WORLD_SCALE;

    matrixPerspective(
        viewport->matProj.m, 
        -aspect_ratio * tan_fov * near,
        aspect_ratio * tan_fov * near,
        tan_fov * near,
        -tan_fov * near,
        near,
        far
    );
    t3d_viewport_set_w_normalize(viewport, near, far);

    struct Transform inverse;
    transformInvert(&scene->camera_transform, &inverse);
    vector3Scale(&inverse.position, &inverse.position, WORLD_SCALE);
    transformToMatrix(&inverse, viewport->matCamera.m);
    viewport->_isCamProjDirty = true;

    t3d_viewport_attach(viewport);
    
    rdpq_set_mode_standard();
    rdpq_mode_persp(true);
    rdpq_mode_zbuf(true, true);
	rdpq_mode_dithering(DITHER_SQUARE_INVSQUARE);
    t3d_state_set_drawflags(T3D_FLAG_DEPTH | T3D_FLAG_SHADED | T3D_FLAG_TEXTURED);

    T3DMat4FP* mtx_fp = frame_pool_get_transformfp(pool);
    T3DMat4 mtx;
    t3d_mat4_identity(&mtx);
    t3d_mat4_scale(&mtx, MODEL_WORLD_SCALE, MODEL_WORLD_SCALE, MODEL_WORLD_SCALE);
    t3d_mat4_to_fixed(mtx_fp, &mtx);
    t3d_matrix_push(mtx_fp);
    rspq_block_run(scene->static_meshes.block);
    t3d_matrix_pop(1);  

    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_t* part = &scene->repair_parts[i];
        repair_part_render(part, pool);
    }
    
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    repair_scene_render_cursor(scene);
    
    rdpq_sync_pipe();
}

repair_part_t* repair_find_part(repair_scene_t* scene) {
    ray_t ray;
    float tan_fov = tanf(scene->camera_fov * 0.5f);
    
    vector3_t local_direction = {
        tan_fov * (2.0f * scene->screen_cursor.x - SCREEN_WD) * (1.0f / SCREEN_HT),
        -tan_fov * (2.0f * scene->screen_cursor.y - SCREEN_HT) * (1.0f / SCREEN_HT), 
        -1.0f,
    };

    vector3Normalize(&local_direction, &local_direction);

    ray.origin = scene->camera_transform.position;
    quatMultVector(&scene->camera_transform.rotation, &local_direction, &ray.dir);

    repair_part_t* result = NULL;
    float min_distance = REALLY_FAR;

    for (int i = 0; i < scene->repair_part_count; i += 1) {
        if (repair_part_raycast(&scene->repair_parts[i], &ray, &min_distance)) {
            result = &scene->repair_parts[i];
        }
    }

    return result;
}

#define CURSOR_SPEED    300.0f

void repair_scene_update(void* data) {
    repair_scene_t* scene = (repair_scene_t*)data;

    joypad_inputs_t input = joypad_get_inputs(0);

    scene->screen_cursor.x += input.stick_x * (CURSOR_SPEED / 80) * fixed_time_step;
    scene->screen_cursor.y -= input.stick_y * (CURSOR_SPEED / 80) * fixed_time_step;

    scene->screen_cursor.x = clampf(scene->screen_cursor.x, 0.0f, 320.0f);
    scene->screen_cursor.y = clampf(scene->screen_cursor.y, 0.0f, 240.0f);

    repair_part_t* hover_part =  repair_find_part(scene);

    debugf("%08x\n", (int)hover_part);
}

repair_scene_t* repair_scene_load(const char* filename) {
    repair_scene_t* result = malloc(sizeof(repair_scene_t));

    result->screen_cursor = (vector2_t){160.0f, 120.0f};

    if (!result) {
        return NULL;
    }

    FILE* file = asset_fopen(filename, NULL);

    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);

    tmesh_load(&result->static_meshes, file);
    fread(&result->repair_part_count, sizeof(uint16_t), 1, file);
    result->repair_parts = malloc(sizeof(repair_part_t) * result->repair_part_count);
    assert(result->repair_parts);

    for (int i = 0; i < result->repair_part_count; i += 1) {
        repair_part_load(&result->repair_parts[i], file);
    }

    fread(&result->camera_transform.position, sizeof(vector3_t), 1, file);
    fread(&result->camera_transform.rotation, sizeof(quaternion_t), 1, file);
    result->camera_transform.scale = gOneVec;

    fread(&result->camera_fov, sizeof(float), 1, file);

    fclose(file);

    material_load_file(&result->assets.cursor_material, "rom:/materials/repair/cursor.mat");

    update_add(result, repair_scene_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    return result;
}

void repair_scene_destroy(repair_scene_t* scene) {
    update_remove(scene);

    material_release(&scene->assets.cursor_material);

    tmesh_release(&scene->static_meshes);
    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_destroy(&scene->repair_parts[i]);
    }
    free(scene->repair_parts);
}