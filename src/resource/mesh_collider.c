#include "mesh_collider.h"

#include <malloc.h>
#include <assert.h>

// CMSH
#define EXPECTED_HEADER 0x434D5348

void mesh_collider_load(struct mesh_collider* into, FILE* file) {
    int header;
    fread(&header, 1, 4, file);
    assert(header == EXPECTED_HEADER);
    
    fread(&into->index.min, sizeof(vector3_t), 1, file);
    fread(&into->index.size_inv, sizeof(vector3_t), 1, file);

    uint16_t node_size;
    uint16_t triangle_count;
    uint16_t vertex_count;

    fread(&node_size, 2, 1, file);
    fread(&triangle_count, 2, 1, file);
    fread(&vertex_count, 2, 1, file);

    int data_size = node_size + triangle_count * sizeof(mesh_triangle_indices_t) + vertex_count * sizeof(struct Vector3);
    void* data = malloc(data_size);
    fread(data, data_size, 1, file);

    into->index.nodes = data;
    into->index.indices = (mesh_triangle_indices_t*)((char*)data + node_size);
    into->index.vertices = (vector3_t*)(into->index.indices + triangle_count);
}

void mesh_collider_release(struct mesh_collider* mesh) {
    free(mesh->index.nodes);
}