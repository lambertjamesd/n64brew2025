import sys
import os

from . import material
import struct

COMMAND_EOF = 0
COMMAND_COMBINE = 1
COMMAND_BLEND = 2
COMMAND_ENV = 3
COMMAND_PRIM = 4
COMMAND_BLEND_COLOR = 5
COMMAND_FLAGS = 6
COMMAND_PALETTE = 7
COMMAND_UV_GEN = 8
COMMAND_FOG = 9
COMMAND_FOG_COLOR = 10
COMMAND_FOG_RANGE = 11
COMMAND_LIGHT_COUNT = 12

T3D_FLAG_DEPTH      = 1 << 0
T3D_FLAG_TEXTURED   = 1 << 1
T3D_FLAG_SHADED     = 1 << 2
T3D_FLAG_CULL_FRONT = 1 << 3
T3D_FLAG_CULL_BACK  = 1 << 4

def _serialize_color(file, color: material.Color):
    file.write(struct.pack('>BBBB', color.r, color.g, color.b, color.a))

COMB_A = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "ONE": 6,
    "1": 6,
    "NOISE": 7,
    "ZERO": 8,
    "0": 8,
}

COMB_B = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "KEYCENTER": 6,
    "K4": 7,
    "ZERO": 8,
    "0": 8,
}

COMB_C = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "KEYSCALE": 6,
    "COMBINED_ALPHA": 7,
    "TEX0_ALPHA": 8,
    "TEX1_ALPHA": 9,
    "PRIM_ALPHA": 10,
    "SHADE_ALPHA": 11,
    "ENV_ALPHA": 12,
    "LOD_FRAC": 13,
    "PRIM_LOD_FRAC": 14,
    "K5": 15,
    "ZERO": 16,
    "0": 16,
}

COMB_D = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    
    "ONE": 6,
    "1": 6,
    "ZERO": 7,
    "0": 7,
}

ACOMB_A = {
    "COMBINED": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    
    "ONE": 6,
    "1": 6,
    "ZERO": 7,
    "0": 7,
}

ACOMB_MUL = {
    "LOD_FRAC": 0,
    "TEX0": 1,
    "TEX1": 2,
    "PRIM": 3,
    "SHADE": 4,
    "ENV": 5,
    "PRIM_LOD_FRAC": 6,
    "ZERO": 7,
    "0": 7,
}

BLEND_A = {
    "IN": 0,
    "MEMORY": 1,
    "BLEND": 2,
    "FOG": 3,
}

BLEND_B1 = {
    "IN_A": 0,
    "FOG_A": 1,
    "SHADE_A": 2,
    "ZERO": 3,
    "0": 3,
}

BLEND_B2 = {
    "INV_MUX_A": 0,
    "MEMORY_CVG": 1,
    'MEM_A': 1,
    "ONE": 2,
    "1": 2,
    "ZERO": 3,
    "0": 3,
}

ZMODE = {
    "OPAQUE": 0 << 10,
    "INTER": 1 << 10,
    "TRANSPARENT": 2 << 10,
    "DECAL": 3 << 10,
}

COVERAGE_DEST = {
    'CLAMP': 0 << 8,
    'WRAP': 1 << 8,
    'ZAP': 2 << 8,
    'FULL': 2 << 8,
    'SAVE': 3 << 8,
}

ALPHACOMPARE = {
    "NONE": 0,
    "THRESHOLD": 1,
    "NOISE": 3,
    "DITHER": 3,
}

UV_GEN = {
    'none': 0,
    'spherical': 1,
}

TEX_FMT = {
    'FMT_NONE': 0,
    'FMT_RGBA16': 2,
    'FMT_RGBA32': 3,
    'FMT_YUV16': 6,
    'FMT_CI4': 8,
    'FMT_CI8': 9,
    'FMT_IA4': 12,
    'FMT_IA8': 13,
    'FMT_IA16': 14,
    'FMT_I4': 16,
    'FMT_I8': 17,
}

RDPQ_COMBINER_2PASS = 1 << 63

def _serialize_combine(file, combine: material.CombineMode, force_cyc2: bool):
    a0 = COMB_A[combine.cyc1.a]
    b0 = COMB_B[combine.cyc1.b]
    c0 = COMB_C[combine.cyc1.c]
    d0 = COMB_D[combine.cyc1.d]

    aa0 = ACOMB_A[combine.cyc1.aa]
    ab0 = ACOMB_A[combine.cyc1.ab]
    ac0 = ACOMB_MUL[combine.cyc1.ac]
    ad0 = ACOMB_A[combine.cyc1.ad]

    a1 = a0
    b1 = b0
    c1 = c0
    d1 = d0

    aa1 = aa0
    ab1 = ab0
    ac1 = ac0
    ad1 = ad0

    flags = 0

    if combine.cyc2:
        a1 = COMB_A[combine.cyc2.a]
        b1 = COMB_B[combine.cyc2.b]
        c1 = COMB_C[combine.cyc2.c]
        d1 = COMB_D[combine.cyc2.d]

        aa1 = ACOMB_A[combine.cyc2.aa]
        ab1 = ACOMB_A[combine.cyc2.ab]
        ac1 = ACOMB_MUL[combine.cyc2.ac]
        ad1 = ACOMB_A[combine.cyc2.ad]

        flags |= RDPQ_COMBINER_2PASS
    elif force_cyc2:
        a1 = COMB_A['0']
        b1 = COMB_B['0']
        c1 = COMB_C['0']
        d1 = COMB_D['COMBINED']

        aa1 = ACOMB_A['0']
        ab1 = ACOMB_A['0']
        ac1 = ACOMB_MUL['0']
        ad1 = ACOMB_A['COMBINED']

        flags |= RDPQ_COMBINER_2PASS


    file.write(struct.pack('>Q', \
        (a0 << 52) | (b0 << 28) | (c0 << 47) | (d0 << 15) | \
        (aa0 << 44) | (ab0 << 12) | (ac0 << 41) | (ad0 << 9) | \
        (a1 << 37) | (b1 << 24) | (c1 << 32) | (d1 << 6) |     \
        (aa1 << 21) | (ab1 << 3) | (ac1 << 18) | (ad1 << 0) | \
        flags \
    ))

SOM_Z_COMPARE = 1 << 4
SOM_Z_WRITE = 1 << 5
SOM_READ_ENABLE = 1 << 6
SOM_BLENDING = 1 << 14

def _serialize_blend(file, blend: material.BlendMode, force_cyc2: bool):
    a1 = BLEND_A[blend.cyc1.a1]
    b1 = BLEND_B1[blend.cyc1.b1]
    a2 = BLEND_A[blend.cyc1.a2]
    b2 = BLEND_B2[blend.cyc1.b2]

    a1_2 = a1
    b1_2 = b1
    a2_2 = a2
    b2_2 = b2

    other_flags = ZMODE[blend.z_mode] | ALPHACOMPARE[blend.alpha_compare]

    if blend.coverage_dest:
        other_flags |= COVERAGE_DEST[blend.coverage_dest]

    if blend.force_blend:
        other_flags |= SOM_BLENDING

    if blend.cyc1.needs_read():
        other_flags |= SOM_READ_ENABLE

    if blend.z_compare:
        other_flags |= SOM_Z_COMPARE

    if blend.z_write:
        other_flags |= SOM_Z_WRITE

    if blend.cyc2:
        a1_2 = BLEND_A[blend.cyc2.a1]
        b1_2 = BLEND_B1[blend.cyc2.b1]
        a2_2 = BLEND_A[blend.cyc2.a2]
        b2_2 = BLEND_B2[blend.cyc2.b2]

        if blend.cyc2.needs_read():
            other_flags |= SOM_READ_ENABLE
    elif force_cyc2:
        a1_2 = a1
        b1_2 = b1
        a2_2 = a2
        b2_2 = b2

        a1 = BLEND_A['IN']
        b1 = BLEND_B1['0']
        a2 = BLEND_A['IN']
        b2 = BLEND_B2['1']

    file.write(struct.pack('>L',
        (a1 << 30) | (b1 << 26) | (a2 << 22) | (b2 << 18) |
        (a1_2 << 28) | (b1_2 << 24) | (a2_2 << 20) | (b2_2 << 16) |
        other_flags    
    ))


def _serialze_string(file, text):
    encoded_text = text.encode()
    file.write(len(encoded_text).to_bytes(1, 'big'))
    file.write(encoded_text)

def _serialize_tex_axis(file, axis: material.TexAxis):
    file.write(b'\x01' if axis.clamp else b'\x00')
    file.write(b'\x01' if axis.mirror else b'\x00')
    file.write(int(axis.mask & 0xFF).to_bytes(1, 'big'))
    file.write(int(axis.shift & 0xFF).to_bytes(1, 'big'))

def _serialize_tex(file, tex: material.Tex, prev_tex: material.Tex = None):
    if not tex:
        file.write(b'\0')
        return
    
    file.write(b'\x01')

    if tex.filename and len(tex.filename):
        _serialze_string(file, tex.rom_filename())
    else:
        file.write(b'\0')
        
    file.write(tex.palette.to_bytes(1, 'big'))
    _serialize_tex_axis(file, tex.s)
    _serialize_tex_axis(file, tex.t)

    file.write(tex.tmem_addr.to_bytes(2, 'big'))
    file.write(TEX_FMT[tex.fmt].to_bytes(2, 'big'))

    file.write(tex.s.min.to_bytes(2, 'big'))
    file.write(tex.t.min.to_bytes(2, 'big'))
    file.write(tex.s.max.to_bytes(2, 'big'))
    file.write(tex.t.max.to_bytes(2, 'big'))

    file.write(tex.width.to_bytes(2, 'big'))
    file.write(tex.height.to_bytes(2, 'big'))

    if tex.frames:
        file.write(len(tex.frames).to_bytes(2, 'big'))
        for frame in tex.frame_filenames():
            _serialze_string(file, frame)
    else:
        file.write(b'\0\0')

    file.write(struct.pack('>ff', tex.s.scroll, -tex.t.scroll))

def _serialize_palette(file, palette: list, palette_offset: int):
    file.write(COMMAND_PALETTE.to_bytes(1, 'big'))
    file.write(palette_offset.to_bytes(2, 'big'))
    file.write(len(palette).to_bytes(2, 'big'))
    for color in palette:
        file.write(color.to_bytes(2, 'big'))

def flags_for_material(mat: material.Material) -> int:
    flags = 0

    if mat.blend_mode and (mat.blend_mode.z_compare or mat.blend_mode.z_write):
        flags |= T3D_FLAG_DEPTH

    if mat.z_buffer:
        flags |= T3D_FLAG_DEPTH

    if mat.culling == 'front':
        flags |= T3D_FLAG_CULL_FRONT
    elif mat.culling == True:
        flags |= T3D_FLAG_CULL_BACK

    if mat.tex0 or mat.tex1:
        flags |= T3D_FLAG_TEXTURED

    if mat.combine_mode and mat.combine_mode.uses('SHADE'):
        flags |= T3D_FLAG_SHADED

    if mat.fog and mat.fog.enabled:
        flags |= T3D_FLAG_SHADED

    return flags

def serialize_material_file(output, mat: material.Material, current_state: material.Material | None = None):
    output.write('MATR'.encode())

    _serialize_tex(output, mat.tex0)
    _serialize_tex(output, mat.tex1, mat.tex0)

    force_cyc2 = False

    if mat.combine_mode and mat.combine_mode.cyc2:
        force_cyc2 = True

    if mat.blend_mode and mat.blend_mode.cyc2:
        force_cyc2 = True

    if mat.fog:
        force_cyc2 = True
    
    if mat.combine_mode:
        output.write(COMMAND_COMBINE.to_bytes(1, 'big'))
        _serialize_combine(output, mat.combine_mode, force_cyc2)

    if mat.blend_mode:
        output.write(COMMAND_BLEND.to_bytes(1, 'big'))
        _serialize_blend(output, mat.blend_mode, force_cyc2)
    
    if mat.env_color:
        output.write(COMMAND_ENV.to_bytes(1, 'big'))
        _serialize_color(output, mat.env_color)

    if mat.prim_color:
        output.write(COMMAND_PRIM.to_bytes(1, 'big'))
        _serialize_color(output, mat.prim_color)

    if mat.blend_color:
        output.write(COMMAND_BLEND_COLOR.to_bytes(1, 'big'))
        _serialize_color(output, mat.blend_color)

    palette_data, palette_offset = mat.get_palette_data()

    if palette_data:
        _serialize_palette(output, palette_data, palette_offset)

    if mat.vtx_effect:
        output.write(COMMAND_UV_GEN.to_bytes(1, 'big'))
        output.write(UV_GEN[mat.vtx_effect].to_bytes(1, 'big'))

    if mat.fog:
        output.write(COMMAND_FOG.to_bytes(1, 'big'))
        output.write((mat.fog.enabled and 1 or 0).to_bytes(1, 'big'))

        if not mat.fog.use_global:
            if mat.fog.fog_color:
                output.write(COMMAND_FOG_COLOR.to_bytes(1, 'big'))
                _serialize_color(output, mat.fog.fog_color)

            if mat.fog.min_distance != mat.fog.max_distance:
                output.write(COMMAND_FOG_RANGE.to_bytes(1, 'big'))
                output.write(int(mat.fog.min_distance).to_bytes(2, 'big'))
                output.write(int(mat.fog.max_distance).to_bytes(2, 'big'))

    flags = flags_for_material(mat)

    if current_state:
        flags |= flags_for_material(current_state)

    output.write(COMMAND_FLAGS.to_bytes(1, 'big'))
    output.write(flags.to_bytes(2, 'big'))

    if mat.light_count != None:
        output.write(COMMAND_LIGHT_COUNT.to_bytes(1, 'big'))
        output.write(mat.light_count.to_bytes(1, 'big'))

    output.write(COMMAND_EOF.to_bytes(1, 'big'))


def serialize_material(filename, mat: material.Material):
    with open(filename, 'wb') as output:
        serialize_material_file(output, mat)