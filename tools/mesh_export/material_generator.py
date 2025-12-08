import bpy
import mathutils
import bmesh
import sys
import math
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.material

script_args = sys.argv[sys.argv.index('--') + 1:]

blend_file = script_args[0]
material_files = script_args[1:]

row_size = math.ceil(math.sqrt(len(material_files)))

def generate_shade_node(node_cache: dict[str, bpy.types.NodeSocket], material: bpy.types.Material, key: str):
    if key in node_cache:
        return node_cache[key]
    
    vertex_color: bpy.types.ShaderNodeVertexColor = material.node_tree.nodes.new('ShaderNodeVertexColor')
    vertex_color.layer_name = 'Color'
    vertex_color.location = (-1000, 200)

    node_cache['SHADE'] = vertex_color.outputs['Color']
    node_cache['SHADE_ALPHA'] = vertex_color.outputs['Alpha']

    return node_cache[key]

def generate_tex_node(node_cache: dict[str, bpy.types.NodeSocket], material: bpy.types.Material, source: entities.material.Material, key: str):
    if key in node_cache:
        return node_cache[key]
    
    image: bpy.types.ShaderNodeTexImage = material.node_tree.nodes.new('ShaderNodeTexImage')
    image.location = (-1000, 0)

    with_tex = source.tex0

    if key[0:4] == 'TEX1':
        with_tex = source.tex1
        image.location = (-1000, -400)

    if with_tex:
        filename = with_tex.filename

        if not filename and with_tex.frames:
            filename = with_tex.frames[0]

        image_path = os.path.join(os.getcwd(), filename)
        image.image = bpy.data.images.load(image_path)
        if with_tex.s.mirror or with_tex.t.mirror:
            image.extension = "MIRROR"

    # image.image = bpy.data.images.load()

    node_cache[key[0:4]] = image.outputs['Color']
    node_cache[f"{key[0:4]}_ALPHA"] = image.outputs['Alpha']

    return node_cache[key]

def link_cycle_option(node_cache: dict[str, bpy.types.NodeSocket], value: str, material: bpy.types.Material, source: entities.material.Material, input: bpy.types.NodeSocket):
    if value == 'COMBINED':
        material.node_tree.links.new(input, generate_combine_cycle(node_cache, material, source, 0))
    if value == 'COMBINED_ALPHA':
        material.node_tree.links.new(input, generate_combine_alpha_cycle(node_cache, material, source, 0))
    elif value == 'TEX0':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX0')
        material.node_tree.links.new(input, tex_output)
    elif value == 'TEX0_ALPHA':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX0_ALPHA')
        material.node_tree.links.new(input, tex_output)
    elif value == 'TEX1':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX1')
        material.node_tree.links.new(input, tex_output)
    elif value == 'TEX1_ALPHA':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX1_ALPHA')
        material.node_tree.links.new(input, tex_output)
    elif value == 'PRIM':
        color = source.prim_color
        if not color:
            color = entities.material.Color(255, 255, 255, 255)
        input.default_value = (color.r / 255, color.g / 255, color.b / 255, color.a / 255)
    elif value == 'PRIM_ALPHA':
        color = source.prim_color
        if not color:
            color = entities.material.Color(255, 255, 255, 255)
        input.default_value = (color.a / 255, color.a / 255, color.a / 255, color.a / 255)
    elif value == 'SHADE':
        shade_output = generate_shade_node(node_cache, material, 'SHADE')
        material.node_tree.links.new(input, shade_output)
    elif value == 'SHADE_ALPHA':
        shade_output = generate_shade_node(node_cache, material, 'SHADE_ALPHA')
        material.node_tree.links.new(input, shade_output)
    elif value == 'ENV':
        color = source.env_color
        if not color:
            color = entities.material.Color(255, 255, 255, 255)
        input.default_value = (color.r / 255, color.g / 255, color.b / 255, color.a / 255)
    elif value == 'ENV_ALPHA':
        color = source.env_color
        if not color:
            color = entities.material.Color(255, 255, 255, 255)
        input.default_value = (color.a / 255, color.a / 255, color.a / 255, color.a / 255)
    elif value == 'NOISE' or value == '1' or value == 'ONE':
        input.default_value = (1, 1, 1, 1)
    elif value == '0' or value == 'ZERO':
        input.default_value = (0, 0, 0, 1)

def link_alpha_cycle_option(node_cache: dict[str, bpy.types.NodeSocket], value: str, material: bpy.types.Material, source: entities.material.Material, input: bpy.types.NodeSocket):
    if value == 'COMBINED':
        material.node_tree.links.new(input, generate_combine_alpha_cycle(node_cache, material, source, 0))
    elif value == 'TEX0':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX0_ALPHA')
        material.node_tree.links.new(input, tex_output)
    elif value == 'TEX1':
        tex_output = generate_tex_node(node_cache, material, source, 'TEX1_ALPHA')
        material.node_tree.links.new(input, tex_output)
    elif value == 'PRIM':
        if source.prim_color:
            input.default_value = source.prim_color.a / 255
        else:
            input.default_value = 1
    elif value == 'SHADE':
        shade_output = generate_shade_node(node_cache, material, 'SHADE_ALPHA')
        material.node_tree.links.new(input, shade_output)
    elif value == 'ENV':
        if source.env_color:
            input.default_value = source.env_color.a / 255
        else:
            input.default_value = 1
    elif value == '1' or value == 'ONE':
        input.default_value = 1
    elif value == '0' or value == 'ZERO':
        input.default_value = 0

def generate_combine_cycle(node_cache: dict[str, bpy.types.NodeSocket], material: bpy.types.Material, source: entities.material.Material, cycleIndex: int):
    cache_name = f"cyc{cycleIndex}"

    if cache_name in node_cache:
        return node_cache[cache_name]

    links = material.node_tree.links
    
    ab: bpy.types.ShaderNodeMixRGB = material.node_tree.nodes.new('ShaderNodeMixRGB')
    ab.blend_type = 'SUBTRACT'
    ab.label = 'A - B'
    ab.location = (-600, -200 - cycleIndex * 200)
    ab.inputs['Fac'].default_value = 1

    mul: bpy.types.ShaderNodeMixRGB = material.node_tree.nodes.new('ShaderNodeMixRGB')
    mul.blend_type = 'MULTIPLY'
    mul.label = ' * C'
    mul.location = (-400, -200 - cycleIndex * 200)
    mul.inputs['Fac'].default_value = 1

    links.new(mul.inputs['Color1'], ab.outputs['Color'])

    add: bpy.types.ShaderNodeMixRGB = material.node_tree.nodes.new('ShaderNodeMixRGB')
    add.blend_type = 'ADD'
    add.label = ' + D'
    add.location = (-200, -200 - cycleIndex * 200)
    add.inputs['Fac'].default_value = 1

    links.new(add.inputs['Color1'], mul.outputs['Color'])

    node_cache[cache_name] = add.outputs['Color']

    cyc = source.combine_mode.cyc1

    if cycleIndex == 1:
        cyc = source.combine_mode.cyc2

    link_cycle_option(node_cache, cyc.a, material, source, ab.inputs['Color1'])
    link_cycle_option(node_cache, cyc.b, material, source, ab.inputs['Color2'])
    link_cycle_option(node_cache, cyc.c, material, source, mul.inputs['Color2'])
    link_cycle_option(node_cache, cyc.d, material, source, add.inputs['Color2'])

    return add.outputs['Color']

def generate_combine_alpha_cycle(node_cache: dict[str, bpy.types.NodeSocket], material: bpy.types.Material, source: entities.material.Material, cycleIndex: int):
    cache_name = f"cyc{cycleIndex}_alpha"

    if cache_name in node_cache:
        return node_cache[cache_name]

    links = material.node_tree.links
    
    ab: bpy.types.ShaderNodeMath = material.node_tree.nodes.new('ShaderNodeMath')
    ab.operation = 'SUBTRACT'
    ab.label = 'A - B'
    ab.location = (-600, 400 - cycleIndex * 200)
    ab.use_clamp = False

    mul: bpy.types.ShaderNodeMath = material.node_tree.nodes.new('ShaderNodeMath')
    mul.operation = 'MULTIPLY'
    mul.label = ' * C'
    mul.location = (-400, 400 - cycleIndex * 200)
    mul.use_clamp = False

    links.new(mul.inputs[0], ab.outputs['Value'])

    add: bpy.types.ShaderNodeMath = material.node_tree.nodes.new('ShaderNodeMath')
    add.operation = 'ADD'
    add.label = ' + D'
    add.location = (-200, 400 - cycleIndex * 200)
    add.use_clamp = True

    links.new(add.inputs[0], mul.outputs['Value'])

    node_cache[cache_name] = add.outputs['Value']

    cyc = source.combine_mode.cyc1

    if cycleIndex == 1:
        cyc = source.combine_mode.cyc2

    link_alpha_cycle_option(node_cache, cyc.aa, material, source, ab.inputs[0])
    link_alpha_cycle_option(node_cache, cyc.ab, material, source, ab.inputs[1])
    link_alpha_cycle_option(node_cache, cyc.ac, material, source, mul.inputs[1])
    link_alpha_cycle_option(node_cache, cyc.ad, material, source, add.inputs[1])

    return add.outputs['Value']

def generate_material(idx: int, material: bpy.types.Material, source: entities.material.Material):
    bpy.ops.mesh.primitive_plane_add(location=((idx % row_size) * 3, (idx // row_size) * 3, 0))

    bpy.ops.object.material_slot_add()
    bpy.context.object.active_material_index = 0
    bpy.context.object.active_material = material
    bpy.ops.object.material_slot_assign()

    bpy.ops.object.mode_set()

    bpy.ops.geometry.color_attribute_add(name = 'Color', color = (1, 1, 1, 1))

    material.use_nodes = True

    node_cache = {}

    if source.blend_mode:
        if source.blend_mode.alpha_compare == 'THRESHOLD':
            material.blend_method = 'CLIP'
        elif source.blend_mode.z_mode == 'TRANSPARENT':
            material.blend_method = 'BLEND'

    output_node = None

    for node in material.node_tree.nodes:
        if node.bl_idname == 'ShaderNodeBsdfPrincipled':
            output_node = node

    if not output_node:
        raise Exception('Could not find ShaderNodeBsdfPrincipled')
    
    output_node.inputs['Emission Strength'].default_value = 1
    output_node.inputs['Base Color'].default_value = (0, 0, 0, 1)

    if source.combine_mode:
        if source.combine_mode.cyc2:
            color_output = generate_combine_cycle(node_cache, material, source, 1)
            alpha_output = generate_combine_alpha_cycle(node_cache, material, source, 1)
        else:
            color_output = generate_combine_cycle(node_cache, material, source, 0)
            alpha_output = generate_combine_alpha_cycle(node_cache, material, source, 0)

        material.node_tree.links.new(output_node.inputs['Emission Color'], color_output)
        material.node_tree.links.new(output_node.inputs['Alpha'], alpha_output)

    # None should default to true
    if source.culling == False:
        material.use_backface_culling = False
    else:
        material.use_backface_culling = True


for obj in bpy.data.objects:
    bpy.data.objects.remove(obj)

for idx, material_filename in enumerate(material_files):
    print(f"generating material for {material_filename}")
    material_source = entities.material.parse_material(material_filename)

    material_name = material_filename[len("assets/"):-len(".mat.json")]
    material = bpy.data.materials.new(material_name)

    material.use_fake_user = True

    generate_material(idx, material, material_source)

# bpy.ops.view3d.toggle_shading(type = 'MATERIAL')

bpy.ops.wm.save_as_mainfile(filepath=blend_file)