#ifndef __PARTICLES_STATIC_PARTICLES_H__
#define __PARTICLES_STATIC_PARTICLES_H__

#include "../render/render_batch.h"
#include <stdio.h>
#include <t3d/t3dmath.h>

#define UNPACK_SCALE(scale) ((float)(scale) * (1.0f / 0xFFFF))

struct static_particles {
    struct material* material;
    struct Vector3 center;
    struct Vector3 size;
    render_batch_particles_t particles;
};

typedef struct static_particles static_particles_t;

static_particles_t* static_particles_load(uint16_t* count, FILE* file);
void static_particles_release(static_particles_t* particles, int count);

void static_particles_start();
void static_particles_end();

void static_particles_render(render_batch_particles_t* particles, T3DMat4FP* transform, bool has_tex);

#endif