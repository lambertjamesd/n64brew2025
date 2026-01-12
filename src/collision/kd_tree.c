#include "kd_tree.h"

#include <assert.h>

#define MAX_DEPTH   16

uint16_t kd_tree_transform_value(float min, float size_inv, float input) {
    float result = size_inv * (input - min);

    if (result > 0xFFFF) {
        return 0xFFFF;
    } 
    if (result < 0) {
        return 0;
    }
    return (uint16_t)result;
}

void kd_tree_transform_arr(kd_tree_t* tree, float* input, uint16_t* output) {
    float* min = VECTOR3_AS_ARRAY(&tree->min);
    float* size_inv = VECTOR3_AS_ARRAY(&tree->size_inv);
    for (int i = 0; i < 3; i += 1) {
        output[i] = kd_tree_transform_value(min[i], size_inv[i], input[i]);
    }
}

bool kd_tree_lookup(kd_tree_t* tree, struct Box3D* box, void *data, kd_triangle_callback callback, int collision_layers) {
    kd_tree_node_t nodes[MAX_DEPTH];
    uint16_t node_offset[MAX_DEPTH];
    kd_tree_node_t* curr = &nodes[0];
    uint16_t* curr_offset = &node_offset[0];
    *curr = *(kd_tree_node_t*)tree->nodes;
    *curr_offset = 0;

    uint16_t box_min[3];
    uint16_t box_max[3];

    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(&box->min), box_min);
    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(&box->max), box_max);
    
    if (box_max[0] == 0 || box_max[1] == 0 || box_max[2] == 0 ||
        box_min[0] == 0xFFFF || box_min[1] == 0xFFFF || box_min[2] == 0xFFFF) {
        return false;
    }

    bool did_hit = false;

    while (curr >= nodes) {
        switch (curr->branch.node_type) {
            case KD_TREE_LEAF_NODE: {
                int max_index = curr->leaf.triangle_offset + curr->leaf.triangle_count;

                for (int i = curr->leaf.triangle_offset; i < max_index; i += 1) {
                    if (callback(data, i, collision_layers)) {
                        did_hit = true;
                    }
                }

                --curr;
                --curr_offset;
                break;
            }
            case KD_TREE_BRANCH_NODE: {
                kd_tree_node_t* next = curr + 1;
                uint16_t* next_offset = curr_offset + 1;
                curr->branch.node_type = KD_TREE_BRANCH_SECOND_NODE;

                assert (next < &nodes[MAX_DEPTH]);

                if (curr->branch.a_max >= box_min[curr->branch.axis]) {
                    *next = *(kd_tree_node_t*)((char*)tree->nodes + *curr_offset + sizeof(kd_tree_branch_t));
                    *next_offset = *curr_offset + sizeof(kd_tree_branch_t);
                    ++curr;
                    ++curr_offset;
                }
                break;
            }
            case KD_TREE_BRANCH_SECOND_NODE:
                if (curr->branch.b_min <= box_max[curr->branch.axis]) {
                    *curr_offset += curr->branch.b_offset;
                    *curr = *(kd_tree_node_t*)((char*)tree->nodes + *curr_offset);
                } else {
                    --curr;
                    --curr_offset;
                }
                break;
        } 
    }

    return did_hit;
}

bool mesh_triangle_shadow_cast(struct mesh_triangle_indices indices, struct Vector3* vertices, struct Vector3* starting_point, struct mesh_shadow_cast_result* result);

bool kd_tree_shadow_cast(kd_tree_t* tree, struct Vector3* starting_point, struct mesh_shadow_cast_result* result) {
    kd_tree_node_t nodes[MAX_DEPTH];
    uint16_t node_offset[MAX_DEPTH];
    kd_tree_node_t* curr = &nodes[0];
    uint16_t* curr_offset = &node_offset[0];
    *curr = *(kd_tree_node_t*)tree->nodes;
    *curr_offset = 0;

    uint16_t pos_local[3];

    kd_tree_transform_arr(tree, VECTOR3_AS_ARRAY(starting_point), pos_local);
    bool has_hit = false;

    while (curr >= nodes) {
        switch (curr->branch.node_type) {
            case KD_TREE_LEAF_NODE: {
                int max_index = curr->leaf.triangle_offset + curr->leaf.triangle_count;

                for (int i = curr->leaf.triangle_offset; i < max_index; i += 1) {
                    struct mesh_shadow_cast_result check;
                    if (mesh_triangle_shadow_cast(tree->indices[i], tree->vertices, starting_point, &check)) {
                        if (!has_hit || check.y > result->y) {
                            *result = check;
                            has_hit = true;
                        }
                    }
                }

                --curr;
                --curr_offset;
                break;
            }
            case KD_TREE_BRANCH_NODE: {
                kd_tree_node_t* next = curr + 1;
                uint16_t* next_offset = curr_offset + 1;
                curr->branch.node_type = KD_TREE_BRANCH_SECOND_NODE;

                assert (next < &nodes[MAX_DEPTH]);

                if (curr->branch.axis == 1 || pos_local[curr->branch.axis] <= curr->branch.a_max) {
                    *next = *(kd_tree_node_t*)((char*)tree->nodes + *curr_offset + sizeof(kd_tree_branch_t));
                    *next_offset = *curr_offset + sizeof(kd_tree_branch_t);
                    ++curr;
                    ++curr_offset;
                }
                break;
            }
            case KD_TREE_BRANCH_SECOND_NODE:
                if (pos_local[curr->branch.axis] >= curr->branch.b_min) {
                    *curr_offset += curr->branch.b_offset;
                    *curr = *(kd_tree_node_t*)((char*)tree->nodes + *curr_offset);
                } else {
                    --curr;
                    --curr_offset;
                }
                break;
            default:
                assert(false);
        } 
    }

    return has_hit;
}