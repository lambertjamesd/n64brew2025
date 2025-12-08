#ifndef __COLLISION_KD_TREE_H__
#define __COLLISION_KD_TREE_H__

#include "../math/box3d.h"
#include "../math/vector3.h"
#include "mesh_index.h"
#include <stdint.h>
#include <stdbool.h>

enum kd_tree_node_type {
    KD_TREE_LEAF_NODE,
    KD_TREE_BRANCH_NODE,

    KD_TREE_BRANCH_SECOND_NODE,
};

struct kd_tree_branch {
    uint8_t node_type;
    uint8_t axis;
    uint16_t a_max;
    uint16_t b_min;
    uint16_t b_offset;
};

typedef struct kd_tree_branch kd_tree_branch_t;

struct kd_tree_leaf {
    uint8_t node_type;
    uint8_t triangle_count;
    uint16_t triangle_offset;
};

typedef struct kd_tree_leaf kd_tree_leaf_t;

union kd_tree_node {
    kd_tree_branch_t branch;
    kd_tree_leaf_t leaf;
};

typedef union kd_tree_node kd_tree_node_t;

struct kd_tree {
    struct Vector3 min;
    struct Vector3 size_inv;
    void* nodes;
    mesh_triangle_indices_t* indices;
    struct Vector3* vertices;
};

typedef struct kd_tree kd_tree_t;

typedef bool (*kd_triangle_callback)(void* data, int triangle_index, int collision_layers);

bool kd_tree_lookup(kd_tree_t* tree, struct Box3D* box, void *data, kd_triangle_callback callback, int collision_layers);
bool kd_tree_shadow_cast(kd_tree_t* tree, struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

#endif