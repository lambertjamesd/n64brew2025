#include "repair_part.h"

#include "../collision/raycast.h"
#include "../cutscene/expression_evaluate.h"
#include "repair_scene.h"

#define REALLY_FAR  10000000.0f

void repair_part_load(repair_part_t* part, FILE* file) {
    vector3_t pos;
    quaternion_t rot;

    fread(&pos, sizeof(pos), 1, file);
    fread(&rot, sizeof(rot), 1, file);

    part->transform = (transform_t){pos, rot, gOneVec};
    
    fread(&part->end_position, sizeof(pos), 1, file);
    fread(&part->end_rotation, sizeof(rot), 1, file);

    tmesh_load(&part->mesh, file);

    uint16_t vertex_count;
    fread(&vertex_count, sizeof(vertex_count), 1, file);
    fread(&part->collider.triangle_count, sizeof(uint16_t), 1, file);

    part->collider.vertices = malloc(sizeof(vector3_t) * vertex_count);
    part->collider.indices = malloc(sizeof(uint16_t) * 3 * part->collider.triangle_count);

    fread(part->collider.vertices, sizeof(vector3_t), vertex_count, file);
    fread(part->collider.indices, sizeof(int16_t), 3 * part->collider.triangle_count, file);

    fread(&part->has_part, sizeof(boolean_variable), 1, file);

    part->target_rotation = part->transform.rotation;
    part->is_connected = false;
    part->is_present = expression_get_bool(part->has_part);
}

void repair_part_destroy(repair_part_t* part) {
    tmesh_release(&part->mesh);
}

void repair_part_render(repair_part_t* part, struct frame_memory_pool* pool) {
    if (part->is_present) {
        material_apply(part->mesh.material);
    } else {
        material_apply(&current_repair_scene->assets.missing_part_material);
    }

    T3DMat4FP* mtx_fp = frame_pool_get_transformfp(pool);
    T3DMat4 mtx;
    transformToWorldMatrix(&part->transform, mtx.m);
    t3d_mat4_to_fixed(mtx_fp, &mtx);
    t3d_matrix_push(mtx_fp);
    rspq_block_run(part->mesh.block);
    t3d_matrix_pop(1);
}

bool repair_part_raycast(repair_part_t* part, ray_t* ray, float* distance) {
    if (!part->is_present) {
        return false;
    }

    transform_t inv;
    transformInvert(&part->transform, &inv);
    ray_t local_ray;
    rayTransform(&inv, ray, &local_ray);

    uint16_t* indices = part->collider.indices;

    float start_distance = *distance;

    for (int i = 0; i < part->collider.triangle_count; i += 1) {
        triangle_raycast(&local_ray, part->collider.vertices, indices, distance);
        indices += 3;
    }

    return *distance != start_distance;
}

void repair_part_update(repair_part_t* part) {
    if (!part->is_present) {
        return;
    }

    quatLerp(&part->transform.rotation, &part->target_rotation, 0.3f, &part->transform.rotation);
}

void repair_part_set_complete(repair_part_t* part) {
    part->is_present = true;
    part->is_connected = true;
    part->transform.position = part->end_position;
    part->transform.rotation = part->end_rotation;
    part->target_rotation = part->end_rotation;
}