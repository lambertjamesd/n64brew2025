#include "static_particles.h"

#include "../resource/material_cache.h"

static_particles_t* static_particles_load(uint16_t* count, FILE* file) {
    uint32_t total_particle_size;
    fread(&total_particle_size, sizeof(uint32_t), 1, file);
    uint16_t static_particle_count;
    fread(&static_particle_count, sizeof(uint16_t), 1, file);

    if (!total_particle_size || !static_particle_count) {
        *count = 0;
        return NULL;
    }

    TPXParticle* curr = malloc(total_particle_size);
    fread(curr, total_particle_size, 1, file);

    static_particles_t* static_particles = malloc(sizeof(static_particles_t) * static_particle_count);

    for (int i = 0; i < static_particle_count; i += 1) {
        static_particles_t* particles = &static_particles[i];

        particles->material = material_cache_load_from_file(file);

        fread(&particles->center, sizeof(struct Vector3), 1, file);
        fread(&particles->size, sizeof(struct Vector3), 1, file);
        particles->particles.particles = curr;

        fread(&particles->particles.particle_count, sizeof(uint16_t), 1, file);
        fread(&particles->particles.particle_size, sizeof(uint16_t), 1, file);
        fread(&particles->particles.particle_scale_width, sizeof(uint16_t), 1, file);
        fread(&particles->particles.particle_scale_height, sizeof(uint16_t), 1, file);

        curr += (particles->particles.particle_count + 1) >> 1;
    }

    *count = static_particle_count;
    return static_particles;
}

void static_particles_release(static_particles_t* particles, int count) {
    for (int i = 0; i < count; i += 1) {
        material_cache_release(particles[i].material);
    }
    if (count) {
        free(particles[0].particles.particles);
    }
    free(particles);
}

void static_particles_start() {
    tpx_state_from_t3d();
    rdpq_mode_zoverride(true, 0, 0);
    rdpq_mode_persp(false);
}

void static_particles_end() {
    rdpq_mode_zoverride(false, 0, 0);
    rdpq_mode_persp(true);
}

void static_particles_render(render_batch_particles_t* particles, T3DMat4FP* transform, bool has_tex) {
    tpx_matrix_push(transform);
    tpx_state_set_base_size(particles->particle_size);
    tpx_state_set_scale(
        UNPACK_SCALE(particles->particle_scale_width),
        UNPACK_SCALE(particles->particle_scale_height)
    );
    tpx_state_set_tex_params(0, 0);
    if (has_tex) {
        tpx_particle_draw_tex(particles->particles, particles->particle_count);
    } else {
        tpx_particle_draw(particles->particles, particles->particle_count);
    }
    tpx_matrix_pop(1);
}