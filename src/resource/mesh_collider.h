#ifndef __RESOURCE_COLLISION_MESH_H__
#define __RESOURCE_COLLISION_MESH_H__

#include "../collision/mesh_collider.h"
#include <stdio.h>

void mesh_collider_load(struct mesh_collider* into, FILE* meshFile);
void mesh_collider_release(struct mesh_collider* mesh);

#endif