#include "scale_in_fade_out.h"

#include "../time/time.h"
#include "../render/render_scene.h"
#include "../render/render_batch.h"
#include "../render/defs.h"

#include "effect_allocator.h"

#define SCALE_IN_TIME   0.25f
#define FADE_OUT_TIME   0.5f

void scale_in_fade_out_render(void* data, struct render_batch* batch) {
    struct scale_in_fade_out* effect = (struct scale_in_fade_out*)data;

    float alpha;

    if (effect->end_time) {
        alpha = 1.0f - (game_time - effect->end_time) * (1.0f / FADE_OUT_TIME);
    } else {
        alpha = 1.0f;
    }

    if (alpha < 0.0f) {
        return;
    }

    T3DMat4FP* mtxfp = render_batch_get_transformfp(batch);

    if (!mtxfp) {
        return;
    }

    float scale = (game_time - effect->start_time) * (1.0f / SCALE_IN_TIME);

    if (scale > 1.0f) {
        scale = 1.0f;
    }

    mat4x4 mtx;
    effect->transform.scale = effect->radius * scale;
    transformSAToMatrix(&effect->transform, mtx);
    render_batch_relative_mtx(batch, mtx);
    t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);

    element_attr_t attrs[2];
    attrs[0].type = ELEMENT_ATTR_PRIM_COLOR;
    attrs[0].color = (color_t){255, 255, 255, (uint8_t)(255.0f * alpha)};
    attrs[1].type = ELEMENT_ATTR_NONE;

    render_batch_add_tmesh(batch, effect->mesh, mtxfp, NULL, NULL, attrs);
}

void scale_in_fade_out_free(struct scale_in_fade_out* effect) {
    render_scene_remove(effect);
    effect_free(effect);
}

struct scale_in_fade_out* scale_in_fade_out_new(struct tmesh* mesh, struct Vector3* pos, struct Vector3* direction, float radius) {
    struct scale_in_fade_out* result = effect_malloc(sizeof(struct scale_in_fade_out));

    result->start_time = game_time;
    result->end_time = 0.0f;

    result->transform.position = *pos;
    vector2LookDir(&result->transform.rotation, direction);
    result->transform.scale = 0.0f;
    result->radius = radius;

    result->mesh = mesh;

    render_scene_add(&result->transform.position, radius, scale_in_fade_out_render, result);

    return result;
}

void scale_in_fade_out_set_transform(struct scale_in_fade_out* effect, struct Vector3* pos, struct Vector3* direction, float radius) {
    effect->transform.position = *pos;
    vector2LookDir(&effect->transform.rotation, direction);
    effect->radius = radius;
}

void scale_in_fade_out_stop(struct scale_in_fade_out* effect) {
    effect->end_time = game_time;
}

bool scale_in_fade_out_is_running(struct scale_in_fade_out* effect) {
    return !effect->end_time || game_time - effect->end_time < FADE_OUT_TIME;
}