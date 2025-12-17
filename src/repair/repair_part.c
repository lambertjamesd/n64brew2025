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