#include "material.h"

#include <t3d/t3d.h>
#include "../resource/sprite_cache.h"
#include "../render/defs.h"

void material_init(struct material* material) {
    material->block = 0;

    material->tex0.sprite = NULL;
    material->tex0.texture_enabled = false;

    material->tex1.sprite = NULL;
    material->tex1.texture_enabled = false;

    material->sort_priority = SORT_PRIORITY_OPAQUE;

    material->palette.tlut = 0;
    material->palette.idx = 0;
    material->palette.size = 0;
    material->flags = 0;
}

void material_destroy(struct material* material) {
    if (material->tex0.sprite) {
        sprite_cache_release(material->tex0.sprite);

        for (int i = 0; i < material->tex0.num_frames; i += 1) {
            sprite_cache_release(material->tex0.frames[i]);
        }

        free(material->tex0.frames);
    }
    if (material->tex1.sprite) {
        sprite_cache_release(material->tex1.sprite);

        for (int i = 0; i < material->tex1.num_frames; i += 1) {
            sprite_cache_release(material->tex1.frames[i]);
        }

        free(material->tex1.frames);
    }
    rspq_block_free(material->block);
    free(material->palette.tlut);
    material->palette.tlut = 0;
}

// MATR
#define EXPECTED_HEADER 0x4D415452

#define COMMAND_EOF         0x00
#define COMMAND_COMBINE     0x01
#define COMMAND_BLEND       0x02
#define COMMAND_ENV         0x03
#define COMMAND_PRIM        0x04
#define COMMAND_BLEND_COLOR 0x05
#define COMMAND_FLAGS       0x06
#define COMMAND_PALETTE     0x07
#define COMMAND_UV_GEN      0x08
#define COMMAND_FOG         0x09
#define COMMAND_FOG_COLOR   0x0A
#define COMMAND_FOG_RANGE   0x0B
#define COMMAND_LIGHT_COUNT 0x0C

#define BIGGEST_MIN_VALUE   (int)(0x3FFF / WORLD_SCALE)

static GLenum material_filter_modes[] = {
    GL_NEAREST,
    GL_LINEAR,
};

void material_load_tex(struct material_tex* tex, FILE* file) {
    uint8_t texture_enabled;
    fread(&texture_enabled, 1, 1, file);

    if (!texture_enabled) {
        tex->texture_enabled = false;
        return;
    }
    tex->texture_enabled = true;

    uint8_t filename_len;
    fread(&filename_len, 1, 1, file);

    char filename[filename_len + 1];
    fread(filename, 1, filename_len, file);
    filename[filename_len] = '\0';

    fread(&tex->params, sizeof(rdpq_tileparms_t), 1, file);

    fread(&tex->tmem_addr, 2, 1, file);
    fread(&tex->fmt, 2, 1, file);
    fread(&tex->s0, 2, 1, file);
    fread(&tex->t0, 2, 1, file);
    fread(&tex->s1, 2, 1, file);
    fread(&tex->t1, 2, 1, file);

    fread(&tex->width, 2, 1, file);
    fread(&tex->height, 2, 1, file);
    
    fread(&tex->num_frames, 2, 1, file);

    if (tex->num_frames) {
        tex->frames = malloc(sizeof(sprite_t*) * tex->num_frames);

        for (int i = 0; i < tex->num_frames; i += 1) {
            uint8_t frame_len;
            fread(&frame_len, 1, 1, file);

            char frame[frame_len + 1];
            fread(frame, 1, frame_len, file);
            frame[frame_len] = '\0';

            tex->frames[i] = sprite_cache_load(frame);
        }
    } else {
        tex->frames = NULL;
    }

    fread(&tex->scroll_x, sizeof(float), 1, file);
    fread(&tex->scroll_y, sizeof(float), 1, file);

    if (filename_len) {
        tex->sprite = sprite_cache_load(filename);
    } else {
        tex->sprite = NULL;
    }
}

int material_size_to_clamp(int size) {
    int clamp = 0;

    while (size) {
        clamp += 1;
        size >>= 1;
    }

    return clamp;
}

void material_upload_tex(rdpq_tile_t tile, struct material_tex* tex) {
    surface_t surf = sprite_get_pixels(tex->sprite);
    rdpq_texparms_t tex_parms = {
        .palette = tex->params.palette,
        .tmem_addr = tex->tmem_addr,
    };
    rdpq_tex_upload(tile, &surf, &tex_parms);
}

void material_upload_placeholder(rdpq_tile_t tile, struct material_tex* tex) {
    surface_t surf = surface_make_placeholder_linear(tile + 1, tex->fmt, tex->width, tex->height);
    rdpq_texparms_t tex_parms = {
        .palette = tex->params.palette,
        .tmem_addr = tex->tmem_addr,
    };
    rdpq_tex_upload(tile, &surf, &tex_parms);
}

void material_use_tex(rdpq_tile_t tile, struct material_tex* tex) {
    int pitch_shift = tex->fmt == FMT_RGBA32 ? 1 : 0;
    rdpq_set_tile(
        tile, 
        tex->fmt, 
        tex->tmem_addr, 
        ((TEX_FORMAT_PIX2BYTES(tex->fmt, tex->width) >> pitch_shift) + 0x7) & ~0x7, 
        &tex->params
    );

    rdpq_set_tile_size_fx(tile, tex->s0, tex->t0, tex->s1, tex->t1);

    if (tex->fmt == FMT_CI4 || tex->fmt == FMT_CI8) {
        rdpq_mode_tlut(TLUT_RGBA16);
    } else {
        rdpq_mode_tlut(TLUT_NONE);
    }
}

void material_load(struct material* into, FILE* material_file) {
    int header;
    fread(&header, 1, 4, material_file);
    assert(header == EXPECTED_HEADER);

    bool has_more = true;

    material_init(into);

    material_load_tex(&into->tex0, material_file);
    material_load_tex(&into->tex1, material_file);

    rspq_block_begin();

    rdpq_mode_begin();

    rdpq_mode_filter(FILTER_BILINEAR);

    bool has_palette = false;
    
    rdpq_blender_t fog_mode = RDPQ_FOG_STANDARD;

    while (has_more) {
        uint8_t nextCommand;
        fread(&nextCommand, 1, 1, material_file);

        switch (nextCommand) {
            case COMMAND_EOF:
                has_more = false;
                break;
            case COMMAND_COMBINE:
                {
                    rdpq_combiner_t combineMode;
                    fread(&combineMode, sizeof(rdpq_combiner_t), 1, material_file);
                    rdpq_mode_combiner(combineMode);
                }
                break;
            case COMMAND_BLEND:
                {
                    rdpq_blender_t blendMode;
                    fread(&blendMode, sizeof(rdpq_blender_t), 1, material_file);
                    rdpq_mode_blender(blendMode & SOM_BLEND_MASK);

                    fog_mode = blendMode & 0xCCCC0000;
                    fog_mode |= fog_mode >> 2;

                    if (blendMode & SOM_Z_COMPARE) {
                        into->flags |= MATERIAL_FLAGS_Z_READ;
                    }                  

                    if (blendMode & SOM_Z_WRITE) {
                        into->flags |= MATERIAL_FLAGS_Z_WRITE;
                    }

                    if ((blendMode & SOM_ALPHACOMPARE_MASK) != 0) {
                        if ((blendMode & SOM_ALPHACOMPARE_MASK) == SOM_ALPHACOMPARE_THRESHOLD) {
                            rdpq_mode_alphacompare(128);
                        } else {
                            rdpq_mode_alphacompare(-1);
                        }
                    } else {
                        rdpq_mode_alphacompare(0);
                    }

                    int flags = SOM_ZMODE_MASK | SOM_READ_ENABLE | SOM_BLENDING | SOM_COVERAGE_DEST_MASK;
                    rdpq_change_other_modes_raw(flags, blendMode & flags);

                    // TODO check when the zmode is decal
                    if ((blendMode & SOM_ZMODE_MASK) == SOM_ZMODE_DECAL) {
                        into->sort_priority = SORT_PRIORITY_DECAL;
                    }

                    if ((blendMode & SOM_Z_WRITE) == 0) {
                        into->sort_priority = SORT_PRIORITY_TRANSPARENT;
                    }
                }
                break;
            case COMMAND_ENV:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_env_color(color);
                }
                break;
            case COMMAND_PRIM:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_prim_color(color);
                }
                break;
            case COMMAND_BLEND_COLOR:
                {
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_blend_color(color);
                }
                break;
            case COMMAND_FLAGS:
                {
                    uint16_t flags;
                    fread(&flags, 2, 1, material_file);
                    t3d_state_set_drawflags(flags);
                }
                break;
            case COMMAND_PALETTE:
                {
                    fread(&into->palette.idx, 2, 1, material_file);
                    fread(&into->palette.size, 2, 1, material_file);
                    into->palette.tlut = malloc(sizeof(uint16_t) * into->palette.size);
                    fread(into->palette.tlut, sizeof(uint16_t), into->palette.size, material_file);
                    data_cache_index_writeback_invalidate(into->palette.tlut, sizeof(uint16_t) * into->palette.size);
                    rdpq_tex_upload_tlut(into->palette.tlut, into->palette.idx, into->palette.size);
                    has_palette = true;
                }
                break;
            case COMMAND_UV_GEN:
                {
                    uint8_t fn;
                    fread(&fn, 1, 1, material_file);

                    switch (fn) {
                        case T3D_VERTEX_FX_NONE:
                            t3d_state_set_vertex_fx(T3D_VERTEX_FX_NONE, 0, 0);
                            break;
                        case T3D_VERTEX_FX_SPHERICAL_UV:
                            t3d_state_set_vertex_fx(T3D_VERTEX_FX_SPHERICAL_UV, into->tex0.sprite->width, into->tex0.sprite->height);
                            break;
                    }
                }
                break;
            case COMMAND_FOG:
                {
                    uint8_t enabled;
                    fread(&enabled, 1, 1, material_file);
                    t3d_fog_set_enabled(enabled);
                    if (enabled) {
                        rdpq_mode_fog(fog_mode);
                    } else {
                        rdpq_mode_fog(0);
                    }
                }
                break;
            case COMMAND_FOG_COLOR:
                {   
                    color_t color;
                    fread(&color, sizeof(color_t), 1, material_file);
                    rdpq_set_fog_color(color);
                }
                break;
            case COMMAND_FOG_RANGE:
                {
                    uint16_t min;
                    uint16_t max;
                    fread(&min, sizeof(min), 1, material_file);
                    fread(&max, sizeof(max), 1, material_file);

                    // higher than this crashes
                    if (min > BIGGEST_MIN_VALUE) {
                        min = BIGGEST_MIN_VALUE;
                    }

                    t3d_fog_set_range(min * WORLD_SCALE, max * WORLD_SCALE);
                }
                break;
            case COMMAND_LIGHT_COUNT:
                {
                    uint8_t light_count;
                    fread(&light_count, 1, 1, material_file);
                    t3d_light_set_count(light_count);
                }
                break;
        }
    }

    rdpq_mode_end();

    if (into->tex0.texture_enabled) {
        if (into->tex0.sprite) {
            material_upload_tex(TILE0, &into->tex0);
        } else if (into->tex0.num_frames) {
            material_upload_placeholder(TILE0, &into->tex0);
        }

        if (!has_palette && into->tex0.sprite && sprite_get_palette(into->tex0.sprite)) {
            rdpq_tex_upload_tlut(sprite_get_palette(into->tex0.sprite), 0, 16);
        }

        material_use_tex(TILE0, &into->tex0);
    }
    if (into->tex1.texture_enabled) {
        if (into->tex1.sprite) {
            material_upload_tex(TILE1, &into->tex1);
        } else if (into->tex1.num_frames) {
            material_upload_placeholder(TILE1, &into->tex1);
        }

        material_use_tex(TILE1, &into->tex1);
    }

    into->block = rspq_block_end();
}

void material_release(struct material* material) {
    material_destroy(material);
}