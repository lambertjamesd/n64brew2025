#include "static_particles.h"

#include "../resource/material_cache.h"
#include "../render/frame_alloc.h"
#include "../render/defs.h"

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

        fread(&particles->instance_count, sizeof(uint16_t), 1, file);
        particles->instance_data = malloc(sizeof(static_particle_instance_data_t) * particles->instance_count);

        fread(particles->instance_data, sizeof(static_particle_instance_data_t) * particles->instance_count, 1, file);
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
    rdpq_sync_pipe();
    tpx_state_from_t3d();
    rdpq_mode_zoverride(true, 0, 0);
    rdpq_mode_persp(false);
}

void static_particles_end() {
    rdpq_sync_pipe();
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
    if (particles) {
        tpx_particle_draw_tex(particles->particles, particles->particle_count);
    } else {
        tpx_particle_draw(particles->particles, particles->particle_count);
    }
    tpx_matrix_pop(1);
}

void static_particles_render_instances(static_particles_t* particle_list, int particles_count, frame_memory_pool_t* pool, vector3_t* camera_pos) {
    struct Transform transform;
    quatIdent(&transform.rotation);
    material_t* curr_material = NULL;
    
    for (int i = 0; i < particles_count; i += 1) {
        static_particles_t* particles = &particle_list[i];

        for (int instance_index = 0; instance_index < particles->instance_count; instance_index += 1) {
            static_particle_instance_data_t* instance = &particles->instance_data[instance_index];

            float distance = sqrtf(vector3DistSqrd(&instance->center, camera_pos));
        
            if (distance >= MAX_PARTICLE_DISTANCE) {
                continue;;
            }
        
            render_batch_particles_t* batch_particles = &particles->particles;
        
            if (distance >= PARTICLE_FADE_DISTANCE) {
                render_batch_particles_t* fade_particles = frame_malloc(pool, sizeof(render_batch_particles_t));
        
                if (fade_particles) {
                    *fade_particles = *batch_particles;
                    uint32_t scale = (uint32_t)((0xFFFF / (MAX_PARTICLE_DISTANCE - PARTICLE_FADE_DISTANCE)) * (MAX_PARTICLE_DISTANCE - distance));
                    batch_particles = fade_particles;
                    batch_particles->particle_scale_width = (uint16_t)((uint32_t)(scale * batch_particles->particle_scale_width) >> 16);
                    batch_particles->particle_scale_height = (uint16_t)((uint32_t)(scale * batch_particles->particle_scale_height) >> 16);
                }
            }
        
            if (curr_material != particles->material) {
                material_apply(particles->material);
                curr_material = particles->material;
            }
        
        
            transform.position = instance->center;
            vector3Sub(&instance->center, camera_pos, &transform.position);
            vector3Scale(&instance->size, &transform.scale, MODEL_WORLD_SCALE);
            vector3Scale(&transform.position, &transform.position, WORLD_SCALE);
        
            T3DMat4FP* mtxfp = frame_pool_get_transformfp(pool);
        
            if (!mtxfp) {
                return;
            }
        
            mat4x4 mtx;
            transformToMatrix(&transform, mtx);
            t3d_mat4_to_fixed_3x4(mtxfp, (T3DMat4*)mtx);
        
            static_particles_render(batch_particles, mtxfp, particles->material->tex0.sprite != NULL);
        }
    }
}