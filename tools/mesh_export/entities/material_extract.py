import bpy
import os.path
import sys

from . import material
from . import serialize
from . import blend_modes
from . import filename

def _reverse_lookup(mapping: dict, value):
    for entry in mapping.items():
        if entry[1] == value:
            return entry[0]

    raise Exception(f'Could not find value {value} in reverse lookup')

def search_node_input(from_node: bpy.types.Node, input_name: str):
    for input in from_node.inputs:
        if input.name == input_name:
            return input
        
    return None


def search_node_linkage(from_node: bpy.types.Node, input_name: str):
    input = search_node_input(from_node, input_name)

    if not input:
        return None

    if not input.is_linked:
        return None

    for link in input.links:
        return link.from_node
        
    return None

def find_node_of_type(tree_nodes: bpy.types.Nodes, type_name: str):
    for node in tree_nodes:
        if node.type == type_name:
            return node
        
    return None

def color_float_to_int(value):
    result = round(value * 255)

    if result > 255:
        result = 255
    elif result < 0:
        result = 0

    return result

def color_array_to_color(array):
    return material.Color(
        color_float_to_int(array[0] ** 0.454545),
        color_float_to_int(array[1] ** 0.454545),
        color_float_to_int(array[2] ** 0.454545),
        color_float_to_int(array[3])
    )

def determine_material_from_nodes(mat: bpy.types.Material, result: material.Material):
    output_node = find_node_of_type(mat.node_tree.nodes, 'OUTPUT_MATERIAL')

    if not output_node:
        print('Could not find output node for material')
        return

    material_type = search_node_linkage(output_node, 'Surface')

    if not material_type:
        print('No input was specified for output material')
        return

    color_link: bpy.types.NodeSocket | None = None

    if material_type.type == 'BSDF_PRINCIPLED':
        color_link = search_node_input(material_type, 'Base Color')
        emmission_link = search_node_input(material_type, 'Emission Color')
        result.lighting = True

        if not color_link.is_linked and emmission_link.is_linked:
            color_link = emmission_link
            result.lighting = False

    elif material_type.type == 'BSDF_DIFFUSE':
        color_link = search_node_input(material_type, 'Color')
        result.lighting = True
    elif material_type.type == 'EMISSION':
        color_link = search_node_input(material_type, 'Color')
        result.lighting = False
    elif material_type.type == 'MIX':
        color_link = search_node_input(output_node, 'Surface')
        result.lighting = False
    else:
        print(f"The node type {material_type.type} is not supported")
        return

    if not color_link:
        print('couldnt find color link')
        return
    
    # TODO determine alpha

    color_name = 'PRIM'
    
    # solid color
    if not color_link.is_linked:
        color = color_link.default_value
        result.prim_color = color_array_to_color(color)
        color_name = 'PRIM'
    elif color_link.links[0].from_node.type == 'TEX_IMAGE':
        color_node: bpy.types.ShaderNodeTexImage = color_link.links[0].from_node
        color_name = 'TEX0'

        input_filename = sys.argv[1]
        image_path = os.path.normpath(os.path.join(os.path.dirname(input_filename), color_node.image.filepath[2:]))

        result.tex0 = material.Tex()
        result.tex0.set_filename(image_path)

        if color_node.interpolation == 'Nearest':
            result.tex0.min_filter = 'nearest'
            result.tex0.mag_filter = 'nearest'
        else:
            result.tex0.min_filter = 'linear'
            result.tex0.mag_filter = 'linear'

        if color_node.extension == 'REPEAT':
            result.tex0.s.clamp = False
            result.tex0.s.mirror = False
            result.tex0.t.clamp = False
            result.tex0.t.mirror = False
        elif color_node.extension == 'MIRROR':
            result.tex0.s.clamp = False
            result.tex0.s.mirror = True
            result.tex0.t.clamp = False
            result.tex0.t.mirror = True
        else:
            result.tex0.s.clamp = True
            result.tex0.s.mirror = False
            result.tex0.t.clamp = True
            result.tex0.t.mirror = False

    if result.lighting:
        result.combine_mode = material.CombineMode(
            material.CombineModeCycle(
                color_name, '0', 'SHADE', '0',
                color_name, '0', 'SHADE', '0'
            ),
            None
        )
    else:
        result.combine_mode = material.CombineMode(
            material.CombineModeCycle(
                '0', '0', '0', color_name,
                '0', '0', '0', color_name
            ),
            None
        )

    if mat.use_backface_culling:
        result.culling = True
    else:
        result.culling = False

    if mat.blend_method == 'CLIP':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'THRESHOLD'
        result.blend_color = material.Color(0, 0, 0, 128)
    elif mat.blend_method == 'BLEND':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', 'IN_A', 'MEMORY', 'INV_MUX_A'), None)
        result.blend_mode.z_write = False
    elif mat.blend_method == 'HASHED':
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)
        result.blend_mode.alpha_compare = 'NOISE'
    else:
        result.blend_mode = material.BlendMode(material.BlendModeCycle('IN', '0', 'IN', '1'), None)

    if 'decal' in mat and mat['decal']:
        result.blend_mode.z_mode = 'DECAL'
        result.blend_mode.z_write = False

def _determine_combiner_from_f3d(combiner) -> material.CombineModeCycle:
     return material.CombineModeCycle(
        _reverse_lookup(serialize.COMB_A, combiner['A']),
        _reverse_lookup(serialize.COMB_B, combiner['B']),
        _reverse_lookup(serialize.COMB_C, combiner['C']),
        _reverse_lookup(serialize.COMB_D, combiner['D']),
        _reverse_lookup(serialize.ACOMB_A, combiner['A_alpha']),
        _reverse_lookup(serialize.ACOMB_A, combiner['B_alpha']),
        _reverse_lookup(serialize.ACOMB_MUL, combiner['C_alpha']),
        _reverse_lookup(serialize.ACOMB_A, combiner['D_alpha']),
    )

def _determine_blend_from_f3d(rdp_settings, index) -> material.BlendModeCycle:
    return material.BlendModeCycle(
        _reverse_lookup(serialize.BLEND_A,rdp_settings[f'blend_p{index}']),
        _reverse_lookup(serialize.BLEND_B1,rdp_settings[f'blend_a{index}']),
        _reverse_lookup(serialize.BLEND_A,rdp_settings[f'blend_m{index}']),
        _reverse_lookup(serialize.BLEND_B1,rdp_settings[f'blend_b{index}']),
    )

def _determine_color_from_f3d(color) -> material.Color:
    return material.Color(
        int((color[0] ** 0.454545) * 255),
        int((color[1] ** 0.454545) * 255),
        int((color[2] ** 0.454545) * 255),
        int(color[3] * 255)
    )

def _determine_tex_axis_from_f3d(axis, image_size, uv_scroll, result: material.TexAxis):
    result.shift = axis['shift']

    result.mask = material.log_pow_2(image_size)
    if 'clamp' in axis and axis['clamp']:
        result.clamp = True
    
    if 'mirror' in axis and axis['mirror']:
        result.mirror = True

    if uv_scroll and uv_scroll['animType'] == 1:
        result.scroll = uv_scroll['speed'] if 'speed' in uv_scroll else 1

    result.min = 0
    result.max = image_size << 2


def _determine_tex_from_f3d(tex, uv_scroll, base_path: str) -> material.Tex | None:
    image = tex['tex']

    if not tex['tex_set'] or not image:
        return None

    result = material.Tex()
    result.set_filename(os.path.normpath(os.path.join(os.path.dirname(base_path), image.filepath[2:])))

    _determine_tex_axis_from_f3d(tex['S'], result.width, uv_scroll['x'] if uv_scroll and 'x' in uv_scroll else None, result.s)
    _determine_tex_axis_from_f3d(tex['T'], result.height, uv_scroll['y'] if uv_scroll and 'y' in uv_scroll else None, result.t)

    if 'tile_scroll' in tex:
        s_scroll = tex['tile_scroll'].get('s') or 0
        t_scroll = tex['tile_scroll'].get('t') or 0

        interval = tex['tile_scroll'].get('interval') or 1

        if s_scroll and interval:
            result.s.scroll += s_scroll / interval
        if t_scroll and interval:
            result.t.scroll += t_scroll / interval

    return result

_CYCLE_1_PRESETS = [
    blend_modes.RM_ZB_OPA_SURF,
    blend_modes.RM_AA_ZB_OPA_SURF,
    blend_modes.RM_AA_ZB_OPA_DECAL,
    blend_modes.RM_AA_ZB_OPA_INTER,
    blend_modes.RM_AA_ZB_TEX_EDGE,
    blend_modes.RM_AA_ZB_XLU_SURF,
    blend_modes.RM_AA_ZB_XLU_DECAL,
    blend_modes.RM_AA_ZB_XLU_INTER,
    blend_modes.RM_FOG_SHADE_A,
    blend_modes.RM_FOG_PRIM_A,
    blend_modes.RM_PASS,
    blend_modes.RM_ADD,
    blend_modes.RM_NOOP,
    blend_modes.RM_ZB_OPA_SURF,
    blend_modes.RM_ZB_OPA_DECAL,
    blend_modes.RM_ZB_XLU_SURF,
    blend_modes.RM_ZB_XLU_DECAL,
    blend_modes.RM_OPA_SURF,
    blend_modes.RM_ZB_CLD_SURF,
    blend_modes.RM_AA_ZB_TEX_TERR,
]

_CYCLE_2_PRESETS = [
    blend_modes.RM_ZB_OPA_SURF,
    blend_modes.RM_AA_ZB_OPA_SURF,
    blend_modes.RM_AA_ZB_OPA_DECAL,
    blend_modes.RM_AA_ZB_OPA_INTER,
    blend_modes.RM_AA_ZB_TEX_EDGE,
    blend_modes.RM_AA_ZB_XLU_SURF,
    blend_modes.RM_AA_ZB_XLU_DECAL,
    blend_modes.RM_AA_ZB_XLU_INTER,
    blend_modes.RM_ADD,
    blend_modes.RM_NOOP,
    blend_modes.RM_ZB_OPA_SURF,
    blend_modes.RM_ZB_OPA_DECAL,
    blend_modes.RM_ZB_XLU_SURF,
    blend_modes.RM_ZB_XLU_DECAL,
    blend_modes.RM_ZB_CLD_SURF,
    blend_modes.RM_ZB_OVL_SURF,
    blend_modes.RM_AA_ZB_TEX_TERR,
    blend_modes.RM_OPA_SURF,
]

enumBlendColor = [
    'IN',
    'MEMORY',
    'BLEND',
    'FOG',
]

enumBlendAlpha = [
    'IN_A',
    'FOG_A',
    'SHADE_A',
    '0',
]

enumBlendMix = [
    'INV_MUX_A',
    'MEM_A',
    '1',
    '0'
]

enumZMode = [
    'OPAQUE',
    'INTER',
    'TRANSPARENT',
    'DECAL',
]

enumAlphaCompare = [
    'NONE',
    'THRESHOLD',
    'DITHER',
]

enumCoverageDest = [
    'CLAMP',
    'WRAP',
    'FULL',
    'SAVE',
]

def determine_materail_blend_f3d(rdp_settings) -> material.BlendMode:
    is_2_cycle = rdp_settings['g_mdsft_cycletype'] == 1

    if rdp_settings['rendermode_advanced_enabled']:
        return material.BlendMode(
            material.BlendModeCycle(enumBlendColor[rdp_settings['blend_p1']], enumBlendAlpha[rdp_settings['blend_a1']], enumBlendColor[rdp_settings['blend_m1']], enumBlendMix[rdp_settings['blend_b1']]),
            material.BlendModeCycle(enumBlendColor[rdp_settings['blend_p2']], enumBlendAlpha[rdp_settings['blend_a2']], enumBlendColor[rdp_settings['blend_m2']], enumBlendMix[rdp_settings['blend_b2']]) if is_2_cycle else None,
            z_mode = enumZMode[rdp_settings['zmode']],
            z_write = rdp_settings['z_upd'],
            z_compare = rdp_settings['z_cmp'],
            aa = rdp_settings['aa_en'],
            alpha_compare = enumAlphaCompare[rdp_settings['g_mdsft_alpha_compare']],
            coverage_dest = enumCoverageDest[rdp_settings['cvg_dst']],
            color_on_coverage = rdp_settings['clr_on_cvg'],
            x_coverage_alpha = rdp_settings['cvg_x_alpha'],
            alpha_coverage = rdp_settings['alpha_cvg_sel'],
            force_blend = rdp_settings['force_bl'],
            image_read = rdp_settings['im_rd'],
        )
    else:
        result = blend_modes.combine_blend_mode(
            _CYCLE_1_PRESETS[rdp_settings['rendermode_preset_cycle_1']],
            _CYCLE_2_PRESETS[rdp_settings['rendermode_preset_cycle_2']] if is_2_cycle else None
        )
        result.alpha_compare = enumAlphaCompare[rdp_settings['g_mdsft_alpha_compare']]
        return result

def determine_material_from_f3d(mat: bpy.types.Material) -> material.Material:
    f3d_mat = mat['f3d_mat']
    rdp_settings = f3d_mat['rdp_settings']

    is_2_cycle = rdp_settings['g_mdsft_cycletype'] == 1

    base_path = sys.argv[1]

    if mat.library:
        base_path = os.path.normpath(os.path.join(os.path.dirname(base_path), mat.library.filepath[2:]))
    
    result: material.Material = material.Material()
    result.combine_mode = material.CombineMode(
        _determine_combiner_from_f3d(f3d_mat['combiner1']),
        _determine_combiner_from_f3d(f3d_mat['combiner2']) if is_2_cycle else None,
    )

    draw_layer = f3d_mat['draw_layer']['sm64']

    if draw_layer == 0 or draw_layer == 1:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'OPAQUE'
    elif draw_layer == 2:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'DECAL'
    elif draw_layer == 3:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.z_mode = 'INTER'
    elif draw_layer == 4:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "IN_A", "MEMORY", "INV_MUX_A"), None)
        result.blend_mode.z_mode = 'TRANSPARENT'
        result.blend_mode.z_write = False
    elif draw_layer == 5 or draw_layer == 6 or draw_layer == 7:
        result.blend_mode = material.BlendMode(material.BlendModeCycle("IN", "IN_A", "MEMORY", "INV_MUX_A"), None)
        result.blend_mode.z_mode = 'TRANSPARENT'
        result.blend_mode.z_write = False

    if rdp_settings['g_mdsft_alpha_compare'] == 1:
        result.blend_mode.alpha_compare = 'THRESHOLD'
    elif rdp_settings['g_mdsft_alpha_compare'] == 2:
        result.blend_mode.alpha_compare = 'NOISE'

    if rdp_settings['set_rendermode']:
        result.blend_mode = determine_materail_blend_f3d(rdp_settings)

    if f3d_mat['set_env']:
        result.env_color = _determine_color_from_f3d(f3d_mat['env_color'])

    if f3d_mat['set_prim']:
        result.prim_color = _determine_color_from_f3d(f3d_mat['prim_color'])

    if f3d_mat['set_blend']:
        result.blend_color = _determine_color_from_f3d(f3d_mat['blend_color'])

    uv_anim_0 = f3d_mat['UVanim0'] if 'UVanim0' in f3d_mat else None
    uv_anim_1 = f3d_mat['UVanim1'] if 'UVanim1' in f3d_mat else None
    result.tex0 = _determine_tex_from_f3d(f3d_mat['tex0'], uv_anim_0, base_path) if result.combine_mode.uses('TEX0') else None
    result.tex1 = _determine_tex_from_f3d(f3d_mat['tex1'], uv_anim_1, base_path) if result.combine_mode.uses('TEX1') else None
    result.layout_textures()

    if rdp_settings['g_cull_back']:
        result.culling = True
    else:
        result.culling = False

    if rdp_settings['g_tex_gen']:
        result.vtx_effect = 'spherical'
    else:
        result.vtx_effect = 'none'

    result.fog = material.Fog()
    result.fog.enabled = rdp_settings['g_fog'] and f3d_mat['set_fog'] and True or False
    result.fog.use_global = f3d_mat['use_global_fog'] and True or False
    result.fog.fog_color = _determine_color_from_f3d(f3d_mat['fog_color'])
    result.fog.min_distance = f3d_mat['fog_position'][0]
    result.fog.max_distance = f3d_mat['fog_position'][1]

    if f3d_mat['set_lights']:
        result.light_count = 1
    else:
        result.light_count = 0

    return result

def material_can_extract(bpy_mat: bpy.types.Material) -> bool:
    return bpy_mat.name.startswith('materials/') or 'f3d_mat' in bpy_mat

def material_romname(bpy_mat: bpy.types.Material) -> str | None:
    if bpy_mat.name.startswith('materials/'):
        material_filename = f"assets/{bpy_mat.name}.mat.json"

        if os.path.exists(material_filename):
            return f"rom:/{bpy_mat.name}.mat"
        else:
            raise Exception(f"{material_filename} does not exist")
        
    if bpy_mat.library:
        return filename.rom_filename(bpy_mat.library.filepath)
    
    return None
        

def load_material_with_name(bpy_mat: bpy.types.Material) -> material.Material:
    if not bpy_mat:
        return material.Material()
    
    material_name = bpy_mat.name

    material_filename = f"assets/{material_name}.mat.json"

    if os.path.exists(material_filename):
        material_object = material.parse_material(material_filename)
        return material_object
    elif 'f3d_mat' in bpy_mat:
        return determine_material_from_f3d(bpy_mat)
    elif not material_name.startswith('materials/'):
        # embedded material
        material_object = material.Material()

        if bpy_mat.use_nodes:
            determine_material_from_nodes(bpy_mat, material_object)

        return material_object
    else:
        raise Exception(f"{material_filename} does not exist")