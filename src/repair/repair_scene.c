#include "repair_scene.h"

#include "../render/defs.h"
#include "../time/time.h"
#include <malloc.h>
#include "../math/mathf.h"
#include "../render/screen_coords.h"
#include "../util/file.h"
#include "../fonts/fonts.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/expression_evaluate.h"
#include "../config.h"

// WRLD
#define EXPECTED_HEADER 0x57524C44
repair_scene_t* current_repair_scene;

static color_t basic_color = {255, 255, 255, 255};
static color_t hover_color = {20, 200, 255, 255};

#define CURSOR_SIZE 32

#define NEAR_PLANE      0.5f
#define REALLY_FAR  10000000.0f

void repair_scene_render_cursor(repair_scene_t* scene) {
    if (!scene->grabbed_part) {
        material_apply(&scene->assets.cursor_material);
    
        int x = (int)scene->screen_cursor.x - CURSOR_SIZE/2;
        int y = (int)scene->screen_cursor.y - CURSOR_SIZE/2;
    
        rdpq_set_prim_color(scene->hovered_part ? hover_color : basic_color);
        rdpq_texture_rectangle(TILE0, x, y, x + CURSOR_SIZE, y + CURSOR_SIZE, 0, 0);
    }
}

void repair_scene_render(repair_scene_t* scene, T3DViewport* viewport, struct frame_memory_pool* pool) {
    rspq_block_run(scene->background_block);

    float tan_fov = tanf(scene->camera_fov * 0.5f);
    float aspect_ratio = (float)viewport->size[0] / (float)viewport->size[1];

    float near = 5.0f * WORLD_SCALE;
    float far = 40.0f * WORLD_SCALE;

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
    
    rdpq_sync_pipe();
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
    
    rdpq_sync_pipe();
    rdpq_mode_persp(false);
    rdpq_set_mode_standard();
    repair_scene_render_cursor(scene);
    
    rdpq_sync_pipe();
}

repair_part_t* repair_find_part(repair_scene_t* scene) {
    ray_t ray;
    screen_coords_to_ray(&scene->camera_transform, scene->camera_fov, &scene->screen_cursor, &ray);

    repair_part_t* result = NULL;
    float min_distance = REALLY_FAR;

    for (int i = 0; i < scene->repair_part_count; i += 1) {
        if (!scene->repair_parts[i].is_connected && repair_part_raycast(&scene->repair_parts[i], &ray, &min_distance)) {
            result = &scene->repair_parts[i];
        }
    }

    return result;
}

#define CURSOR_SPEED    300.0f
#define OBJECT_SPEED    5.0f

static quaternion_t relative_rotations[4] = {
    {-SQRT_1_2_F, 0.0f, 0.0f, SQRT_1_2_F},
    {0.0f, 0.0f, -SQRT_1_2_F, SQRT_1_2_F},
    {SQRT_1_2_F, 0.0f, 0.0f, SQRT_1_2_F},
    {0.0f, 0.0f, SQRT_1_2_F, SQRT_1_2_F},
};

void repair_scene_handle_grabbed_part(repair_scene_t* scene, joypad_inputs_t input, joypad_buttons_t pressed) {
    if (pressed.c_up) {
        quaternion_t new_rotation;
        quatMultiply(&relative_rotations[0], &scene->grabbed_part->target_rotation, &new_rotation);
        scene->grabbed_part->target_rotation = new_rotation;
    }
    if (pressed.c_right) {
        quaternion_t new_rotation;
        quatMultiply(&relative_rotations[1], &scene->grabbed_part->target_rotation, &new_rotation);
        scene->grabbed_part->target_rotation = new_rotation;
    }
    if (pressed.c_down) {
        quaternion_t new_rotation;
        quatMultiply(&relative_rotations[2], &scene->grabbed_part->target_rotation, &new_rotation);
        scene->grabbed_part->target_rotation = new_rotation;
    }
    if (pressed.c_left) {
        quaternion_t new_rotation;
        quatMultiply(&relative_rotations[3], &scene->grabbed_part->target_rotation, &new_rotation);
        scene->grabbed_part->target_rotation = new_rotation;
    }

    vector3_t up;
    vector3_t right;
    quatMultVector(&scene->camera_transform.rotation, &gUp, &up);
    quatMultVector(&scene->camera_transform.rotation, &gRight, &right);

    vector3AddScaled(
        &scene->grabbed_part->transform.position, 
        &right,
        input.stick_x * OBJECT_SPEED * (1.0f / 80.0f) * fixed_time_step,
        &scene->grabbed_part->transform.position
    );
    vector3AddScaled(
        &scene->grabbed_part->transform.position, 
        &up,
        input.stick_y * OBJECT_SPEED * (1.0f / 80.0f) * fixed_time_step,
        &scene->grabbed_part->transform.position
    );
}

#define DROP_TOLERNACE  1.0f

void repair_scene_check_drop(repair_scene_t* scene) {
    repair_part_t* grabbed_part = scene->grabbed_part;

    screen_coords_from_position(&scene->camera_transform, scene->camera_fov, &grabbed_part->transform.position, &scene->screen_cursor);

    scene->grabbed_part = NULL;

    if (fabsf(quatDot(&grabbed_part->transform.rotation, &grabbed_part->end_rotation)) < 0.9f) {
        return;
    }

    ray_t ray_check;
    ray_check.origin = scene->camera_transform.position;
    vector3Sub(&grabbed_part->transform.position, &ray_check.origin, &ray_check.dir);
    vector3Normalize(&ray_check.dir, &ray_check.dir);

    float target_distance = rayDetermineDistance(&ray_check, &grabbed_part->end_position);
    float actual_distnace = rayDetermineDistance(&ray_check, &grabbed_part->transform.position);

    vector3_t pos_check;
    vector3AddScaled(&grabbed_part->transform.position, &ray_check.dir, target_distance - actual_distnace, &pos_check);

    if (vector3DistSqrd(&pos_check, &grabbed_part->end_position) > DROP_TOLERNACE * DROP_TOLERNACE) {
        return;
    }

    grabbed_part->transform.position = grabbed_part->end_position;
    grabbed_part->target_rotation = grabbed_part->end_rotation;
    grabbed_part->is_connected = true;
}

void repair_scene_exit_with_message(repair_scene_t* scene, const char* message) {
    cutscene_builder_t builder;
    cutscene_builder_init(&builder);

    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_dialog(&builder, message);
    cutscene_builder_fade(&builder, FADE_COLOR_BLACK, 1.0f);
    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_load_scene(&builder, scene->exit_scene);
    cutscene_builder_fade(&builder, FADE_COLOR_NONE, 1.0f);

    cutscene_runner_run(cutscene_builder_finish(&builder), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void repair_scene_update(void* data) {
    repair_scene_t* scene = (repair_scene_t*)data;

    joypad_inputs_t input = joypad_get_inputs(0);
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    scene->screen_cursor.x += input.stick_x * (CURSOR_SPEED / 80) * fixed_time_step;
    scene->screen_cursor.y -= input.stick_y * (CURSOR_SPEED / 80) * fixed_time_step;

    scene->screen_cursor.x = clampf(scene->screen_cursor.x, 0.0f, 320.0f);
    scene->screen_cursor.y = clampf(scene->screen_cursor.y, 0.0f, 240.0f);

    scene->hovered_part = repair_find_part(scene);

    if (pressed.a) {
        if (scene->grabbed_part) {
            repair_scene_check_drop(scene);
        } else {
            scene->grabbed_part = scene->hovered_part;
        }
    }

    if (scene->grabbed_part) {
        repair_scene_handle_grabbed_part(scene, input, pressed);
    }

    bool is_complete = true;
    
    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_update(&scene->repair_parts[i]);

        if (!scene->repair_parts[i].is_connected) {
            is_complete = false;
        }
    }

#if ENABLE_CHEATS
    if (pressed.l) {
        is_complete = true;
    }
#endif

    if (!scene->is_complete && is_complete) {
        scene->is_complete = is_complete;
        repair_scene_exit_with_message(scene, "Repair complete");
        expression_set_bool(scene->puzzle_complete, true);
    }
}

static const char* backgrounds[] = {
    [REPAIR_VARIANT_OUTSIDE] = "rom:/images/repair/background.sprite",
    [REPAIR_VARIANT_INSIDE] = "rom:/images/repair/background-inside.sprite",
};

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

    result->is_missing_parts = false;

    tmesh_load(&result->static_meshes, file);
    fread(&result->repair_part_count, sizeof(uint16_t), 1, file);
    result->repair_parts = malloc(sizeof(repair_part_t) * result->repair_part_count);
    assert(result->repair_parts);

    for (int i = 0; i < result->repair_part_count; i += 1) {
        repair_part_load(&result->repair_parts[i], file);
        
        if (!result->repair_parts[i].is_present) {
            result->is_missing_parts = true;
        }
    }

    fread(&result->camera_transform.position, sizeof(vector3_t), 1, file);
    fread(&result->camera_transform.rotation, sizeof(quaternion_t), 1, file);
    result->camera_transform.scale = gOneVec;

    fread(&result->camera_fov, sizeof(float), 1, file);
    fread(&result->puzzle_complete, sizeof(boolean_variable), 1, file);
    result->exit_scene = file_read_string(file);
    fread(&result->variant, 4, 1, file);

    fclose(file);

    material_load_file(&result->assets.cursor_material, "rom:/materials/repair/cursor.mat");
    result->assets.background = sprite_load(backgrounds[result->variant]);

    update_add(result, repair_scene_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    font_type_use(FONT_DIALOG);

    result->is_complete = expression_get_bool(result->puzzle_complete);
    result->grabbed_part = NULL;

    if (result->is_complete) {
        repair_scene_exit_with_message(result, "This has already been repaired");

        for (int i = 0; i < result->repair_part_count; i += 1) {
            repair_part_set_complete(&result->repair_parts[i]);
        }
    } else if (result->is_missing_parts) {
        repair_scene_exit_with_message(result, "You are missing some parts");
    }

    rspq_block_begin();
    
    rdpq_sync_pipe();
    rdpq_set_mode_copy(false);
    rdpq_sprite_blit(result->assets.background, 0.0f, 0.0f, NULL);

    result->background_block = rspq_block_end();

    return result;
}

void repair_scene_destroy(repair_scene_t* scene) {
    rspq_block_free(scene->background_block);
    font_type_release(FONT_DIALOG);

    free(scene->exit_scene);
    update_remove(scene);

    material_release(&scene->assets.cursor_material);
    sprite_free(scene->assets.background);

    tmesh_release(&scene->static_meshes);
    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_destroy(&scene->repair_parts[i]);
    }
    free(scene->repair_parts);
}