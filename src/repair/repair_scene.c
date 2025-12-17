#include "repair_scene.h"

#include "../render/defs.h"
#include <malloc.h>

// WRLD
#define EXPECTED_HEADER 0x57524C44
repair_scene_t* current_repair_scene;

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

    rspq_block_run(scene->static_meshes.block);

    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_t* part = &scene->repair_parts[i];

        T3DMat4FP* mtx_fp = frame_pool_get_transformfp(pool);
        T3DMat4 mtx;
        transformToWorldMatrix(&part->transform, mtx.m);
        t3d_mat4_to_fixed(mtx_fp, &mtx);
        t3d_matrix_push(mtx_fp);
        rspq_block_run(part->mesh.block);
        t3d_matrix_pop(1);  
    }
    
    rdpq_sync_pipe();
}

repair_scene_t* repair_scene_load(const char* filename) {
    repair_scene_t* result = malloc(sizeof(repair_scene_t));

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

    return result;
}

void repair_scene_destroy(repair_scene_t* scene) {
    tmesh_release(&scene->static_meshes);
    for (int i = 0; i < scene->repair_part_count; i += 1) {
        repair_part_destroy(&scene->repair_parts[i]);
    }
    free(scene->repair_parts);
}