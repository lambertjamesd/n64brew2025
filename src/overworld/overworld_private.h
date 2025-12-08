#ifndef __OVERWORLD_PRIVATE_H__
#define __OVERWORLD_PRIVATE_H__

#include "overworld_render.h"

struct overworld_tile_slice {
    uint16_t min_x;
    uint16_t max_x;
    uint16_t y;
    uint16_t has_more;
};

struct overworld_step_state {
    struct Vector2 loop[8];
    int loop_count;
    int left;
    int right;
    float current_y;
    float min_x;
    float max_x;
};

int overworld_create_top_view(struct overworld* overworld, mat4x4 view_proj_matrix, struct Vector3* camera_position, struct Vector2* loop);
struct overworld_tile_slice overworld_step(struct overworld* overworld, struct overworld_step_state* state);


#endif