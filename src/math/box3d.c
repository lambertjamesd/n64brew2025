
#include "box3d.h"

int box3DContainsPoint(struct Box3D* box, struct Vector3* point) {
    return box->min.x < point->x &&
        box->min.y < point->y &&
        box->min.z < point->z &&
        box->max.x > point->x &&
        box->max.y > point->y &&
        box->max.z > point->z;
        
}

int box3DHasOverlap(struct Box3D* a, struct Box3D* b) {
    return a->min.x <= b->max.x && a->max.x >= b->min.x &&
        a->min.z <= b->max.z && a->max.z >= b->min.z &&
        a->min.z <= b->max.z && a->max.z >= b->min.z;
}

void box3DUnion(struct Box3D* a, struct Box3D* b, struct Box3D* out) {
    vector3Max(&a->max, &b->max, &out->max);
    vector3Min(&a->min, &b->min, &out->min);
}

void box3DUnionPoint(struct Box3D* a, struct Vector3* point, struct Box3D* out) {
    vector3Max(&a->max, point, &out->max);
    vector3Min(&a->min, point, &out->min);
}

void box3DExtendDirection(struct Box3D* a, struct Vector3* direction, struct Box3D* out) {
    *out = *a;

    if (direction->x > 0.0) {
        out->max.x += direction->x;
    } else {
        out->min.x += direction->x;
    }

    if (direction->y > 0.0) {
        out->max.y += direction->y;
    } else {
        out->min.y += direction->y;
    }

    if (direction->z > 0.0) {
        out->max.z += direction->z;
    } else {
        out->min.z += direction->z;
    }
}

void box3DSupportFunction(struct Box3D* box, struct Vector3* input, struct Vector3* output) {
    output->x = input->x > 0.0f ? box->max.x : box->min.x;
    output->y = input->y > 0.0f ? box->max.z : box->min.z;
    output->z = input->z > 0.0f ? box->max.z : box->min.z;
} 

void box3DRotate2D(struct Box3D* box, struct Vector2* rotation, struct Box3D* result) {
    int x_pos = rotation->x >= 0;
    int y_pos = rotation->y >= 0;

    struct Vector3* a = x_pos ? &box->max : &box->min;
    struct Vector3* b = x_pos ? &box->min : &box->max;

    struct Vector3* c = y_pos ? &box->min : &box->max;
    struct Vector3* d = y_pos ? &box->max : &box->min;

    result->max.x = a->x * rotation->x 
        - c->z * rotation->y;
    result->max.z = d->x * rotation->y 
        + a->z * rotation->x;
    result->min.x = b->x * rotation->x 
        - d->z * rotation->y;
    result->min.z = c->x * rotation->y 
        + b->z * rotation->x;

    result->min.y = box->min.y;
    result->max.y = box->max.y;
}