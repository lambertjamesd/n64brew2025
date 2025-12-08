#ifndef __OVERWORLD_LOAD_H__
#define __OVERWORLD_LOAD_H__

#include "overworld.h"
#include <stdio.h>

struct overworld_tile* overworld_tile_load(FILE* file);
void overworld_tile_free(struct overworld_tile* tile);

struct overworld_actor_tile* overworld_actor_tile_load(FILE* file);
void overworld_actor_tile_free(struct overworld_actor_tile* tile);

void overworld_check_loaded_tiles(struct overworld* overworld);
void overworld_check_collider_tiles(struct overworld* overworld, struct Vector3* player_pos);

#endif