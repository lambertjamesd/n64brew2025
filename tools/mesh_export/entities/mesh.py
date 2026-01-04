import bpy
import mathutils
import struct
import bmesh
import math

from . import armature

def interpolate_color(a, b, lerp):
    lerp_inv = 1 - lerp
    return [
        a[0] * lerp_inv + b[0] * lerp,
        a[1] * lerp_inv + b[1] * lerp,
        a[2] * lerp_inv + b[2] * lerp,
        a[3] * lerp_inv + b[3] * lerp
    ]

def interpolate_vertex(a, b, lerp):
    lerp_inv = 1 - lerp
    return (
        a[0] * lerp_inv + b[0] * lerp,
        a[1] * lerp_inv + b[1] * lerp,
        interpolate_color(a[2], b[2], lerp),
        a[3] * lerp_inv + b[3] * lerp,
        a[4] # just copy over the first bone
    )

class mesh_data():
    def __init__(self, mat: bpy.types.Material) -> None:
        self.mat: bpy.types.Material = mat
        self.vertices: list[mathutils.Vector] = []
        self.normals: list = []
        self.color: list = []
        self.uv: list = []
        self.bone_indices: list = []
        
        self.indices: list[int] = []

    def copy(self):
        result = mesh_data(self.mat)

        result.vertices = self.vertices.copy()
        result.normals = self.normals.copy()
        result.color = self.color.copy()
        result.uv = self.uv.copy()
        result.indices = self.indices.copy()
        result.bone_indices = self.bone_indices.copy()

        return result
    
    def mat_name(self) -> str:
        if self.mat:
            return self.mat.name
        return ""
    
    def is_empty(self):
        return len(self.indices) == 0
    
    def copy_blank(self):
        return mesh_data(self.mat)
    
    def translate(self, offset: mathutils.Vector):
        for i in range(len(self.vertices)):
            self.vertices[i] = self.vertices[i] + offset

    def scale(self, scale: mathutils.Vector):
        for i in range(len(self.vertices)):
            self.vertices[i] = self.vertices[i] * scale

    def append_vertex(self, vertex_data):
        self.vertices.append(vertex_data[0])
        self.normals.append(vertex_data[1])
        self.color.append(vertex_data[2])
        self.uv.append(vertex_data[3])
        self.bone_indices.append(vertex_data[4])

    def append_triangle(self, a, b, c):
        self.indices.append(a)
        self.indices.append(b)
        self.indices.append(c)

    def get_triangles(self):
        result: list[list[int]] = []

        for triangle_index in range(0, len(self.indices), 3):
            result.append(self.indices[triangle_index:triangle_index+3])

        return result

    def get_vertex(self, index):
        return (self.vertices[index], self.normals[index], self.color[index], self.uv[index], self.bone_indices[index])
    
    def get_vertex_interpolated(self, a, b, lerp):
        return interpolate_vertex(self.get_vertex(a), self.get_vertex(b), lerp)
    
    def center(self) -> mathutils.Vector:
        bb = self.bounding_box()
        return (bb[0] + bb[1]) * 0.5

    def bounding_box(self) -> tuple[mathutils.Vector, mathutils.Vector]:
        if len(self.vertices) == 0:
            return (mathutils.Vector((0, 0, 0)), mathutils.Vector((0, 0, 0)))
        
        min_result = mathutils.Vector(self.vertices[0])
        max_result = mathutils.Vector(self.vertices[0])

        for vertex in self.vertices:
            min_result.x = min(min_result.x, vertex.x)
            min_result.y = min(min_result.y, vertex.y)
            min_result.z = min(min_result.z, vertex.z)

            max_result.x = max(max_result.x, vertex.x)
            max_result.y = max(max_result.y, vertex.y)
            max_result.z = max(max_result.z, vertex.z)

        return [min_result, max_result]

    def append_mesh(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, material_index: int, final_transform: mathutils.Matrix, armature: armature.ArmatureData | None):
        triangles = []
        max_index = -1
        used_indices = set()

        normal_transform = final_transform.to_3x3()
        normal_transform.invert()
        normal_transform.transpose()
        bone_index = -1

        if obj.parent_bone and armature:
            bone = armature.find_bone_data(obj.parent_bone)
            final_transform = bone.pose_bone.matrix.inverted() @ obj.matrix_world
            normal_transform = final_transform.to_3x3().inverted().transposed()
            bone_index = bone.index
            
            # world = parent_inverse @ obj.matrix_world

        for polygon in mesh.polygons:
            if polygon.material_index != material_index:
                continue
            
            for loop_index in polygon.loop_indices:
                max_index = max(max_index, loop_index)
                used_indices.add(loop_index)
                
            triangles.append(polygon.loop_indices[0])
            triangles.append(polygon.loop_indices[1])
            triangles.append(polygon.loop_indices[2])

        next_output = len(self.indices)
        index_mapping = dict()

        uv_layer = None
        
        for layer in mesh.uv_layers:
            if layer.active:
                uv_layer = layer
                break

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

        if not color and any_color:
            color = any_color

        for loop_index in range(max_index + 1):
            if not loop_index in used_indices:
                continue

            vtx_index = mesh.loops[loop_index].vertex_index

            vertex_transform = final_transform
            normal_vertex_transform = normal_transform

            bone_name = None
            group_index = None
            bone = None

            if not armature is None and not obj.parent_bone:
                group_index = -1
                group_weight = 0

                for element in mesh.vertices[vtx_index].groups:
                    if element.weight > group_weight:
                        group_weight = element.weight
                        group_index = element.group

                if group_index != -1:
                    bone_name = obj.vertex_groups[group_index].name
                    bone = armature.find_bone_data(bone_name)
                    bone_index = bone.index

                    vertex_transform = bone.matrix_scene_inv @ final_transform
                    normal_vertex_transform = bone.matrix_normal_inv @ normal_transform

            pos = vertex_transform @ mesh.vertices[vtx_index].co

            self.vertices.append(pos)
            self.normals.append(normal_vertex_transform @ mesh.vertices[vtx_index].normal)

            if uv_layer:
                self.uv.append(uv_layer.uv[loop_index].vector)
            else:
                self.uv.append([0, 0])

            color_vertex = [1, 1, 1, 1]

            if color and color.domain == 'CORNER':
                color_vertex = list(color.data[loop_index].color)
            elif color and color.domain == 'POINT':
                color_vertex = list(color.data[vtx_index].color)
            
            if alpha and alpha.domain == 'CORNER':
                color_vertex[3] = alpha.data[loop_index].color[0]
            elif alpha and alpha.domain == 'POINT':
                color_vertex[3] = alpha.data[vtx_index].color[0]

            self.color.append(color_vertex)

            self.bone_indices.append(bone_index)

            index_mapping[loop_index] = next_output
            next_output += 1
        
        for idx in triangles:
            self.indices.append(index_mapping[idx])

    
def convert_vertex_channel(input, gamma):
    result = math.floor(255 * math.pow(input, gamma) + 0.5)

    if result > 255:
        return 255
    elif result < 0:
        return 0
    
    return result

def pack_vertex(vertex, uv, color, normal, bone_index, gamma = 1):
    result = struct.pack(
        ">hhh", 
        math.floor(vertex[0] * 64 + 0.5), 
        math.floor(vertex[1] * 64 + 0.5), 
        math.floor(vertex[2] * 64 + 0.5)
    )

    if uv:
        result = result + struct.pack(
            ">hh",
            math.floor(uv[0] * 256 + 0.5) % 32768,
            math.floor((1 - uv[1]) * 256 + 0.5) % 32768
        )

    if color:
        result = result + struct.pack(
            ">BBBB", 
            convert_vertex_channel(color[0], gamma), 
            convert_vertex_channel(color[1], gamma), 
            convert_vertex_channel(color[2], gamma), 
            convert_vertex_channel(color[3], 1)
        )

    if normal:
        normal = normal.normalized()
        result = result + struct.pack(
            ">bbb", 
            math.floor(normal[0] * 127 + 0.5), 
            math.floor(normal[1] * 127 + 0.5), 
            math.floor(normal[2] * 127 + 0.5)
        )

    if bone_index != None:
        result = result + struct.pack(
            ">h", 
            bone_index
        )

    return result

class mesh_list_entry:
    def __init__(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.obj: bpy.types.Object = obj
        self.mesh: bpy.types.Mesh = mesh
        self.transform: mathutils.Matrix = transform

class mesh_list():
    def __init__(self, base_transform: mathutils.Matrix) -> None:
        self.meshes: list[mesh_list_entry] = []
        self.base_transform: mathutils.Matrix = base_transform

    def append(self, obj):
        mesh = obj.data
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()
        self.meshes.append(mesh_list_entry(obj, mesh, self.base_transform @ obj.matrix_world))

    def determine_mesh_data(self, armature: armature.ArmatureData | None = None) -> list[mesh_data]:
        mesh_by_material: dict[str, mesh_data] = dict()

        for entry in self.meshes:
            mesh = entry.mesh
            transform = entry.transform

            for material_index in range(max(len(mesh.materials), 1)):
                if material_index < len(mesh.materials):
                    mat = mesh.materials[material_index]

                    if mat == None:
                        continue

                    name = mat.name
                else:
                    continue

                if not name in mesh_by_material:
                    mesh_by_material[name] = mesh_data(mat)

                mesh_data_instance = mesh_by_material[name]

                mesh_data_instance.append_mesh(entry.obj, mesh, material_index, transform, armature)

        all_meshes = list(mesh_by_material.values())

        all_meshes.sort(key=lambda x: x.mat_name())

        return all_meshes
