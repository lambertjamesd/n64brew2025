#include "../test/framework_test.h"
#include "kd_tree.h"

#define PACK_BYTES(a, b)    (((uint16_t)(a) << 8) | ((uint16_t)(b)))

static uint16_t kd_tree_data[] = {
    PACK_BYTES(KD_TREE_BRANCH_NODE, 0),
    0x9000, 0x7000,
    4 * sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),

    PACK_BYTES(KD_TREE_BRANCH_NODE, 1),
    0x9000, 0x7000, 
    2 * sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),
    
    PACK_BYTES(KD_TREE_BRANCH_NODE, 2),
    0x9000, 0x7000, 
    sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),
    
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    0,
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    1,

    PACK_BYTES(KD_TREE_BRANCH_NODE, 2),
    0x9000, 0x7000,
    sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),
    
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    2,
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    3,

    PACK_BYTES(KD_TREE_BRANCH_NODE, 1),
    0x9000, 0x7000,
    2 * sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),

    PACK_BYTES(KD_TREE_BRANCH_NODE, 2),
    0x9000, 0x7000,
    sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),
    
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    4,
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    5,

    PACK_BYTES(KD_TREE_BRANCH_NODE, 2),
    0x9000, 0x7000,
    sizeof(kd_tree_branch_t) + sizeof(kd_tree_leaf_t),

    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    6,
    PACK_BYTES(KD_TREE_LEAF_NODE, 1),
    7,
};

static mesh_triangle_indices_t indicies[] = {
    {
        .indices = {0, 1, 2},
        .surface_type = 0,
    },
    {
        .indices = {3, 4, 5},
        .surface_type = 0,
    },
    {
        .indices = {6, 7, 8},
        .surface_type = 0,
    },
    {
        .indices = {9, 10, 11},
        .surface_type = 0,
    },
    {
        .indices = {12, 13, 14},
        .surface_type = 0,
    },
    {
        .indices = {15, 16, 17},
        .surface_type = 0,
    },
    {
        .indices = {18, 19, 20},
        .surface_type = 0,
    },
    {
        .indices = {21, 22, 23},
        .surface_type = 0,
    },
};

static kd_tree_t kd_tree_test = {
    .min = {-10.0f, -5.0f, -10.0f},
    .size_inv = {0xFFFF / 20.0f, 0xFFFF / 10.0f, 0xFFFF / 20.0f},
    .nodes = &kd_tree_data[0],
    .indices = indicies,
};

struct test_kd_tree_result {
    int call_count;
    int call_mask;
};

void test_kd_tree_result_init(struct test_kd_tree_result* result) {
    result->call_count = 0;
    result->call_mask = 0;
}

bool test_kd_triangle_callback(void* data, int triangle_index, int collision_layers) {
    struct test_kd_tree_result* result = (struct test_kd_tree_result*)data;

    result->call_count += 1;
    result->call_mask += (1 << triangle_index);
    
    return true;
}

void test_kd_tree_lookup(struct test_context* t) {
    struct Box3D lookup_box = {
        {-9.0f, -4.0f, -9.0f},
        {-8.0f, -3.0f, -8.0f},
    };

    struct test_kd_tree_result result;

    test_kd_tree_result_init(&result);
    kd_tree_lookup(&kd_tree_test, &lookup_box, &result, test_kd_triangle_callback, 1);
    test_eqi(t, 1, result.call_count);
    test_eqi(t, 1, result.call_mask);
    
    lookup_box.min.z = 8.0f;
    lookup_box.max.z = 9.0f;
    test_kd_tree_result_init(&result);
    kd_tree_lookup(&kd_tree_test, &lookup_box, &result, test_kd_triangle_callback, 1);
    test_eqi(t, 1, result.call_count);
    test_eqi(t, 2, result.call_mask);
    
    lookup_box.min.z = -1.0f;
    lookup_box.max.z = 1.0f;
    test_kd_tree_result_init(&result);
    kd_tree_lookup(&kd_tree_test, &lookup_box, &result, test_kd_triangle_callback, 1);
    test_eqi(t, 2, result.call_count);
    test_eqi(t, 3, result.call_mask);
}

void test_kd_tree_shadow_cast(struct test_context* t) {
    
}