#include "repair_part.h"

void repair_part_load(repair_part_t* part, FILE* file) {
    vector3_t pos;
    quaternion_t rot;

    fread(&pos, sizeof(pos), 1, file);
    fread(&rot, sizeof(rot), 1, file);

    part->transform = (transform_t){pos, rot, gOneVec};
    
    fread(&part->end_position, sizeof(pos), 1, file);
    fread(&part->end_rotation, sizeof(rot), 1, file);

    tmesh_load(&part->mesh, file);
}

void repair_part_destroy(repair_part_t* part) {
    tmesh_release(&part->mesh);
}

void repair_part_render(repair_part_t* part, struct frame_memory_pool* pool) {
    T3DMat4FP* mtx_fp = frame_pool_get_transformfp(pool);
    T3DMat4 mtx;
    transformToWorldMatrix(&part->transform, mtx.m);
    t3d_mat4_to_fixed(mtx_fp, &mtx);
    t3d_matrix_push(mtx_fp);
    rspq_block_run(part->mesh.block);
    t3d_matrix_pop(1);  
}