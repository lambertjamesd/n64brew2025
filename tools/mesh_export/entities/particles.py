import bpy
import mathutils
import io
import struct
import math

from . import material_extract

def _get_instance_info(obj: bpy.types.Object) -> tuple[bpy.types.Mesh | None, mathutils.Vector | None, float | None]:
    mesh = obj.data

    if mesh == None or not isinstance(mesh, bpy.types.Mesh):
        return None, None, None

    bb = obj.bound_box

    min_y = bb[0][1]
    max_y = min_y

    for vertex in bb:
        min_y = min(min_y, vertex[1])
        max_y = max(max_y, vertex[1])

    return mesh, obj.dimensions, (min_y + max_y) * 0.5

def find_mesh_instance(obj: bpy.types.Object) -> tuple[bpy.types.Mesh | None, mathutils.Vector | None, float | None]:
    if 'particle_instance_obj' in obj and obj['particle_instance_obj']:
        return _get_instance_info(obj['particle_instance_obj'])

    for modifier in obj.modifiers:
        if not isinstance(modifier, bpy.types.NodesModifier):
            continue

        for node in modifier.node_group.nodes:
            if not isinstance(node, bpy.types.GeometryNodeObjectInfo):
                continue

            if not 'Object' in node.inputs:
                continue

            object_instance = node.inputs['Object']

            if object_instance.is_linked:
                continue

            default_value = object_instance.default_value

            if not default_value:
                continue

            if not isinstance(default_value, bpy.types.Object):
                continue

            return _get_instance_info(default_value)

    return None, None, None

def clamp_particle_pos(value: float) -> int:
    result = int(math.floor(value + 0.5))

    if result < -128:
        return 128
    elif result > 127:
        return 127
    
    return result

def transform_particle(point: mathutils.Vector, mid_point: mathutils.Vector, scale_inv: mathutils.Vector, center_offset: float) -> mathutils.Vector:
    pos = (point + mathutils.Vector((0, center_offset, 0)) - mid_point) * scale_inv

    return mathutils.Vector((
        clamp_particle_pos(pos.x),
        clamp_particle_pos(pos.y),
        clamp_particle_pos(pos.z)
    ))

class Particles:
    def __init__(self, obj: bpy.types.Object):
        self.obj: bpy.types.Object = obj
        self.position: mathutils.Vector = mathutils.Vector()
        self.scale: mathutils.Vector = mathutils.Vector()
        self.particle_count: int = 0
        self.particle_size: int = 0
        self.particle_scale_width: int = 0
        self.particle_scale_height: int = 0
        self.particles: bytes
        self.material: bpy.types.Material

    def set_dimensions(self, dimensions: mathutils.Vector):
        scaled = dimensions * 16
        self.particle_size = math.ceil(max(scaled.x, scaled.y))
        self.particle_scale_width = math.floor(0x7FFF * scaled.x / self.particle_size)
        self.particle_scale_height = math.floor(0x7FFF * scaled.y / self.particle_size)

    def write_into(self, file):
        filename_str = material_extract.material_romname(self.material)

        if not filename_str:
            file.write((0).to_bytes(1, 'big'))
            return

        material_filename = filename_str.encode()
        file.write(len(material_filename).to_bytes(1, 'big'))
        file.write(material_filename)
        file.write(struct.pack('>fff', self.position.x, self.position.y, self.position.z))
        file.write(struct.pack('>fff', self.scale.x, self.scale.y, self.scale.z))

        final_count = self.particle_count

        if (final_count % 2) != 0:
            final_count += 1

        file.write(struct.pack('>HHHH', final_count, self.particle_size, self.particle_scale_width, self.particle_scale_height))

def convert_channel(value):
    return round(255 * value)

def pack_color(col):
    return struct.pack(
        '>BBBB', 
        convert_channel(col[0]),
        convert_channel(col[1]),
        convert_channel(col[2]),
        convert_channel(col[3])
    )

def extract_color(index, color, alpha, has_texture):
    result = list(color.data[index].color)

    if has_texture:
        result[3] = 0
    elif alpha:
        result[3] = alpha.data[index].color[0]

    return pack_color(result)

def build_particles(obj: bpy.types.Object, base_transform: mathutils.Matrix) -> Particles | None:
    if obj.type != 'MESH':
        print(f'WARNING: static_particle object should be a mesh for object {obj.name}')
        return None
    
    instance, dimensions, center_offset = find_mesh_instance(obj)

    if not instance or not dimensions:
        print(f'WARNING: could not find mesh instance for static_particle for object {obj.name}')
        return None

    mesh: bpy.types.Mesh = obj.data

    full_transform = base_transform @ obj.matrix_world

    min_pos = None
    max_pos = None

    for vertex in mesh.vertices:
        pos = full_transform @ vertex.co

        pos.y += center_offset
        
        if min_pos is None:
            min_pos = pos
        else:
            min_pos = mathutils.Vector((
                min(min_pos.x, pos.x),
                min(min_pos.y, pos.y),
                min(min_pos.z, pos.z)
            ))

        if max_pos is None:
            max_pos = pos
        else:
            max_pos = mathutils.Vector((
                max(max_pos.x, pos.x),
                max(max_pos.y, pos.y),
                max(max_pos.z, pos.z)
            ))

    scale = (max_pos - min_pos) * 0.5

    scale_inv = mathutils.Vector((
        127 / scale.x if scale.x > 0 else 1,
        127 / scale.y if scale.y > 0 else 1,
        127 / scale.z if scale.z > 0 else 1
    ))

    mid_point = (max_pos + min_pos) * 0.5

    particle_data = io.BytesIO()

    result = Particles(obj)

    result.position = mid_point
    result.scale = scale * 0.5
    result.particle_count = len(mesh.vertices)
    result.set_dimensions(dimensions)
    result.material = mesh.materials[0]

    material = material_extract.load_material_with_name(mesh.materials[0])
    has_texture = material.tex0 != None

    color = None
    alpha = None
    any_color = None

    for attr in mesh.attributes:
        if attr.data_type == 'BYTE_COLOR' or attr.data_type == 'FLOAT_COLOR':
            if attr.name.lower().startswith('col'):
                color = attr
            elif attr.name.lower() == 'alpha':
                alpha = attr
            else:
                any_color = attr

    size = obj.vertex_groups.get("Size")

    if not color and any_color:
        color = any_color

    for index in range(0, len(mesh.vertices), 2):
        has_b = index + 1 < len(mesh.vertices)

        vertex = mesh.vertices[index]
        next_vertex = mesh.vertices[index + 1] if has_b else None

        posA = transform_particle(full_transform @ vertex.co, mid_point, scale_inv, center_offset)

        if next_vertex:
            posB = transform_particle(full_transform @ next_vertex.co, mid_point, scale_inv, center_offset)
        else:
            posB = mathutils.Vector()

        size_a = vertex.groups[size.index].weight if size else 1

        particle_data.write(struct.pack(
            '>bbbb', 
            int(posA.x), int(posA.y), int(posA.z), round(127 * size_a)
        ))

        size_b = next_vertex.groups[size.index].weight if next_vertex and size else 1

        particle_data.write(struct.pack(
            '>bbbb', 
            int(posB.x), int(posB.y), int(posB.z), round(127 * size_b)
        ))

        if color and color.domain == 'POINT':
            particle_data.write(extract_color(index, color, alpha, has_texture))
        else:
            particle_data.write(struct.pack(
                '>BBBB', 
                255, 255, 255, 0 if has_texture else 255
            ))
        if has_b and color and color.domain == 'POINT':
            particle_data.write(extract_color(index + 1, color, alpha, has_texture))
        else:
            particle_data.write(struct.pack(
                '>BBBB', 
                255, 255, 255, 0 if has_texture else 255
            ))

    result.particles = particle_data.getvalue()

    return result

def write_particles(particle_list: list[Particles], file):
    particle_data = io.BytesIO()

    for particles in particle_list:
        particle_data.write(particles.particles)

    particle_data_bytes = particle_data.getvalue()

    file.write(struct.pack('>IH', len(particle_data_bytes), len(particle_list)))
    file.write(particle_data_bytes)

    for particles in particle_list:
        particles.write_into(file)