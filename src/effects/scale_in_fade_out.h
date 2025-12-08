#ifndef __EFFECTS_SCALE_IN_FADE_OUT_H__
#define __EFFECTS_SCALE_IN_FADE_OUT_H__

#include <stdbool.h>
#include "../math/transform_single_axis.h"
#include "../render/tmesh.h"

struct scale_in_fade_out {
    struct tmesh* mesh;
    struct TransformSingleAxis transform;
    float start_time;
    float end_time;
    float radius;
};

struct scale_in_fade_out* scale_in_fade_out_new(struct tmesh* mesh, struct Vector3* pos, struct Vector3* direction, float radius);

void scale_in_fade_out_set_transform(struct scale_in_fade_out* effect, struct Vector3* pos, struct Vector3* direction, float radius);
void scale_in_fade_out_stop(struct scale_in_fade_out* effect);
void scale_in_fade_out_free(struct scale_in_fade_out* effect);
bool scale_in_fade_out_is_running(struct scale_in_fade_out* effect);

#endif