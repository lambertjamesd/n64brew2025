#ifndef __PARTICLES_STATIC_PARTICLES_H__
#define __PARTICLES_STATIC_PARTICLES_H__

#include "../render/render_batch.h"
#include <stdio.h>
#include <t3d/t3dmath.h>

#define UNPACK_SCALE(scale) ((float)(scale) * (1.0f / 0xFFFF))

#define MAX_PARTICLE_DISTANCE 70.0f
#define PARTICLE_FADE_DISTANCE 40.0f

struct static_particle_instance_data {
    struct Vector3 center;
    struct Vector3 size;
};

typedef struct static_particle_instance_data static_particle_instance_data_t;

struct static_particles {
    struct material* material;
    static_particle_instance_data_t* instance_data;
    uint16_t instance_count;
    render_batch_particles_t particles;
};

typedef struct static_particles static_particles_t;

static_particles_t* static_particles_load(uint16_t* count, FILE* file);
void static_particles_release(static_particles_t* particles, int count);

void static_particles_start();
void static_particles_end();

void static_particles_render(render_batch_particles_t* particles, T3DMat4FP* transform, bool has_tex);
void static_particles_render_instances(static_particles_t* particle_list, int particles_count, frame_memory_pool_t* pool, vector3_t* camera_pos);

#endif