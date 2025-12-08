import bpy
import mathutils
import bmesh
import struct
import math
import time

INDEX_BLOCK_SIZE_X = 8
INDEX_BLOCK_SIZE_Y = 16
INDEX_BLOCK_SIZE_Z = 8

ERROR_MARGIN = 0.001

class MeshColliderTriangle():
    def __init__(self, indices: list[int], surface_type: int = 0):
        self.indices: list[int] = indices
        self.surface_type = surface_type
        self.centroid: mathutils.Vector | None = None
        self.min: mathutils.Vector | None = None
        self.max: mathutils.Vector | None = None
        self.unused_edges: int = 0

    def serialize(self, file):
        file.write(struct.pack(
            ">HHHBB",
            self.indices[0],
            self.indices[1],
            self.indices[2],
            self.surface_type,
            self.unused_edges,
        ))

    def calculate_bounds(self, vertices: list[mathutils.Vector]):
        a = vertices[self.indices[0]]
        b = vertices[self.indices[1]]
        c = vertices[self.indices[2]]

        self.centroid = (a + b + c) * (1.0 / 3.0)
        self.min = _vector_min(a, _vector_min(b, c))
        self.max = _vector_max(a, _vector_max(b, c))

    def mark_edge_unused(self, edge_index: int):
        self.unused_edges = self.unused_edges | (1 << edge_index)

def _triangle_support_function(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle, direction: mathutils.Vector):
    result_index = max(triangle.indices, key = lambda index: direction.dot(vertices[index]))
    return vertices[result_index]

def _aabb_support_function(min: mathutils.Vector, max: mathutils.Vector, direction: mathutils.Vector):
    return mathutils.Vector((
        min.x if direction.x < 0 else max.x,
        min.y if direction.y < 0 else max.y,
        min.z if direction.z < 0 else max.z
    ))

_basis_vectors = [
    mathutils.Vector((1, 0, 0)),
    mathutils.Vector((0, 1, 0)),
    mathutils.Vector((0, 0, 1))
]

def _generate_overlap_directions(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle) -> list[mathutils.Vector]:
    triangle_positions: list[mathutils.Vector] = [vertices[index] for index in triangle.indices]

    result: list[mathutils.Vector] = []

    result.append((triangle_positions[1] - triangle_positions[0]).cross(triangle_positions[2] - triangle_positions[0]))

    for basis_vector in _basis_vectors:
        result.append(basis_vector)

        for idx in range(len(triangle_positions)):
            next_idx = (idx + 1) % 3
            result.append(basis_vector.cross(triangle_positions[idx] - triangle_positions[next_idx]))

    return result

def _does_overlap(vertices: list[mathutils.Vector], triangle: MeshColliderTriangle, min: mathutils.Vector, max: mathutils.Vector):
    vector_directions = _generate_overlap_directions(vertices, triangle)

    for vector_direction in vector_directions:
        if vector_direction.magnitude < 0.0000001:
            continue

        dir = vector_direction.normalized()

        max_tri = dir.dot(_triangle_support_function(vertices, triangle, dir))
        min_tri = dir.dot(_triangle_support_function(vertices, triangle, -dir))

        max_box = dir.dot(_aabb_support_function(min, max, dir))
        min_box = dir.dot(_aabb_support_function(min, max, -dir))

        if min_tri > max_box + ERROR_MARGIN or min_box > max_tri + ERROR_MARGIN:
            return False

    return True

def _vector_min(a: mathutils.Vector, b: mathutils.Vector):
    return mathutils.Vector((
            min(a.x, b.x), 
            min(a.y, b.y), 
            min(a.z, b.z),
        ))

def _vector_max(a: mathutils.Vector, b: mathutils.Vector):
    return mathutils.Vector((
            max(a.x, b.x), 
            max(a.y, b.y), 
            max(a.z, b.z),
        ))

def _vector_floor(point: mathutils.Vector):
    return mathutils.Vector((
            math.floor(point.x),
            math.floor(point.y),
            math.floor(point.z),
        ))

def _vector_ceil(point: mathutils.Vector):
    return mathutils.Vector((
            math.ceil(point.x),
            math.ceil(point.y),
            math.ceil(point.z),
        ))

def _determine_index_bounds(points: list[mathutils.Vector]):
    if len(points) == 0:
        return mathutils.Vector(), mathutils.Vector()

    min_point = points[0]
    max_point = points[0]

    for point in points:
        min_point = _vector_min(min_point, _vector_floor(point))
        max_point = _vector_max(max_point, _vector_ceil(point))

    return min_point, max_point

def _determine_index_indices(points: list[mathutils.Vector], force_subdivisions: mathutils.Vector | None = None):
    min_point, max_point = _determine_index_bounds(points)

    if force_subdivisions:
        size = max_point - min_point
        stride_inv = mathutils.Vector((
            force_subdivisions.x / size.x if size.x != 0 else 1, 
            force_subdivisions.y / size.y if size.y != 0 else 1, 
            force_subdivisions.z / size.z if size.z != 0 else 1
        ))
        return min_point, stride_inv, force_subdivisions

    stride_inv = mathutils.Vector((1 / INDEX_BLOCK_SIZE_X, 1 / INDEX_BLOCK_SIZE_Y, 1 / INDEX_BLOCK_SIZE_Z))
    size = (max_point - min_point) * stride_inv
    block_count = mathutils.Vector((math.ceil(size.x), math.ceil(size.y), math.ceil(size.z)))

    return min_point, stride_inv, block_count

CENTER_THREHOLD = 0.001

def _split_loop(loop: list[mathutils.Vector], axis: int, distance: float) -> tuple[list[mathutils.Vector], list[mathutils.Vector]]:
    offsets = list(map(lambda x: x[axis] - distance, loop))

    behind = []
    front = []

    for i in range(len(loop)):
        current_offset = offsets[i]

        if current_offset < CENTER_THREHOLD:
            behind.append(loop[i])

        if current_offset > -CENTER_THREHOLD:
            front.append(loop[i])


        next_index = (i + 1) % len(loop)
        next_offset = offsets[next_index]

        if (current_offset >= CENTER_THREHOLD and next_offset <= -CENTER_THREHOLD) or (current_offset <= -CENTER_THREHOLD and next_offset >= CENTER_THREHOLD):
            lerp_value = -current_offset / (next_offset - current_offset)
            center_point = loop[i] * (1 - lerp_value) + loop[next_index] * lerp_value
            behind.append(center_point)
            front.append(center_point)

    return behind, front

def _rasterize_loop_detrmine_range(points: list[mathutils.Vector], axis: int) -> tuple[int, int]:
    min_result = min(map(lambda point: math.floor(point[axis]), points))
    max_result = max(map(lambda point: math.ceil(point[axis]), points))

    if min_result == max_result:
        max_result += 1

    return min_result, max_result

def _rasterize_loop_axis_3d(points: list[mathutils.Vector], axis: int, result: list[tuple[int, int, int]], prev_axis: int | None):
    curr_value = min(map(lambda point: math.floor(point[axis]), points))

    curr_loop = points

    while len(curr_loop) > 0:
        behind, front = _split_loop(curr_loop, axis, curr_value + 1)

        if len(behind) == 0:
            raise Exception('the behind loop should never be empty')

        if axis == 1:
            min_z, max_z = _rasterize_loop_detrmine_range(behind, 2)
            for z in range(min_z, max_z):
                result.append((prev_axis, curr_value, z))
        else:
            _rasterize_loop_axis_3d(behind, axis + 1, result, curr_value)

        curr_value += 1
        curr_loop = front

        if curr_value > 100:
            print(curr_loop)
            raise Exception('infinite loop here')

    
def _rasterize_triangle_3d(points: list[mathutils.Vector]) -> list[tuple[int, int, int]]:
    result = []
    _rasterize_loop_axis_3d(points, 0, result, None)
    return result

KD_TREE_LEAF_NODE = 0
KD_TREE_BRANCH_NODE = 1

def _transform_value(min, size_inv, value) -> int:
    result = int((value - min) * size_inv)

    if result > 0xFFFF:
        return 0xFFFF
    if result < 0:
        return 0
    return result

class KdNode():
    def __init__(self, vertices: list[mathutils.Vector], triangles: list[MeshColliderTriangle]):
        self.triangles: list[MeshColliderTriangle] | None = None
        self.left = None
        self.right = None
        
        min_point = triangles[0].min if len(triangles) > 0 else mathutils.Vector((0, 0, 0))
        max_point = triangles[0].max if len(triangles) > 0 else mathutils.Vector((0, 0, 0))

        for triangle in triangles:
            min_point = _vector_min(min_point, triangle.min)
            max_point = _vector_max(max_point, triangle.max)

        self.min_point = min_point
        self.max_point = max_point

        size = max_point - min_point
    
        axis = 0

        if size.y > size.x and size.y > size.z:
            axis = 1
        if size.z > size.x and size.z > size.y:
            axis = 2

        self.axis = axis

        if len(triangles) < 4 or size[axis] < 0.5:
            self.triangles = triangles[:256]
            return

        triangles = sorted(triangles, key = lambda x: x.centroid[axis])

        half_index = len(triangles) // 2

        self.left = KdNode(vertices, triangles[0:half_index])
        self.right = KdNode(vertices, triangles[half_index:])
        
    def to_bytes(self, triangle_array, min, size_inv):
        if self.triangles != None:
            result = struct.pack(">BBH", KD_TREE_LEAF_NODE, len(self.triangles), len(triangle_array))
            triangle_array.extend(self.triangles)
            return result

        left_bytes = self.left.to_bytes(triangle_array, min, size_inv)
        right_bytes = self.right.to_bytes(triangle_array, min, size_inv)

        return struct.pack(
            ">BBHHH",
            KD_TREE_BRANCH_NODE,
            self.axis,
            _transform_value(min[self.axis], size_inv[self.axis], self.left.max_point[self.axis]),
            _transform_value(min[self.axis], size_inv[self.axis], self.right.min_point[self.axis]),
            len(left_bytes) + 8
        ) + left_bytes + right_bytes
            
    def to_string(self, indent):
        if self.triangles != None:
            return indent + 'KD_TREE_LEAF_NODE ' + str(len(self.triangles)) + '\n'

        return indent + 'KD_TREE_BRANCH_NODE ' + \
            str(self.axis) + ', ' + str(self.left.max_point[self.axis]) + ', ' + str(self.right.min_point[self.axis]) + '\n' + \
            self.left.to_string(indent + '  ') + \
            self.right.to_string(indent + '  ')

class KdMeshIndex():
    def __init__(self, vertices: list[mathutils.Vector], triangles: list[MeshColliderTriangle]):
        self.vertices: list[mathutils.Vector] = vertices
        self.triangles: list[MeshColliderTriangle] = triangles

        for triangle in self.triangles:
            triangle.calculate_bounds(vertices)

        self.root = KdNode(self.vertices, self.triangles)
        
    def serialize(self, file):
        triangles = []
        min_point = self.root.min_point
        size = self.root.max_point - min_point

        if size.x < 0.1:
            size.x = 0.1
        if size.y < 0.1:
            size.y = 0.1
        if size.z < 0.1:
            size.z = 0.1

        size_inv = 0xFFFF * mathutils.Vector((1 / size.x, 1 / size.y, 1 / size.z))
        nodes_data = self.root.to_bytes(triangles, min_point, size_inv)

        file.write(struct.pack(">fff", min_point.x, min_point.y, min_point.z))
        file.write(struct.pack(">fff", size_inv.x, size_inv.y, size_inv.z))
        file.write(struct.pack(">HHH", len(nodes_data), len(triangles), len(self.vertices)))
        file.write(nodes_data)
        for triangle in triangles:
            triangle.serialize(file)
        for vertex in self.vertices:
            file.write(struct.pack(">fff", vertex.x, vertex.y, vertex.z))

    def __str__(self):
        return self.root.to_string('')

def _edge_key(a_index, b_index) -> str:
    if a_index < b_index:
        return f"{a_index}:{b_index}"
    return f"{b_index}:{a_index}"

class EdgeConnection():
    def __init__(self, triangle: MeshColliderTriangle, edge_index: int, normal: mathutils.Vector):
        self.triangle: MeshColliderTriangle = triangle
        self.edge_index: int = edge_index
        self.normal: mathutils.Vector = normal

class MeshCollider():
    def __init__(self):
        self.vertices: list[mathutils.Vector] = []
        self.triangles: list[MeshColliderTriangle] = []

    def append(self, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)

        for face in bm.faces:
            face.verts
            face.loops

        start_index = len(self.vertices)

        for vert in bm.verts:
            self.vertices.append(transform @ vert.co)

        for face in bm.faces:
            mat = None
            if face.material_index >= 0 and face.material_index < len(mesh.materials):
                mat = mesh.materials[face.material_index]

            surface_type = 0

            if mat and 'surface_type' in mat:
                surface_type = mat['surface_type']
            
            self.triangles.append(MeshColliderTriangle([
                face.verts[0].index + start_index,
                face.verts[1].index + start_index,
                face.verts[2].index + start_index,
            ], surface_type=surface_type))

        bm.free()
    
    def _get_triangle_normal(self, triangle: MeshColliderTriangle) -> mathutils.Vector:
        a = self.vertices[triangle.indices[1]] - self.vertices[triangle.indices[0]]
        b = self.vertices[triangle.indices[2]] - self.vertices[triangle.indices[0]]

        return a.cross(b).normalized()

    def find_needed_edges(self):
        edge_to_face: dict[str, EdgeConnection] = {}

        for triangle in self.triangles:
            normal = self._get_triangle_normal(triangle)

            for index in range(0, 3):
                a_index = triangle.indices[index]
                b_index = triangle.indices[(index + 1) % 3]

                key = _edge_key(a_index, b_index)

                if not (key in edge_to_face):
                    edge_to_face[key] = EdgeConnection(triangle, index, normal)
                    continue

                tangent = (self.vertices[b_index] - self.vertices[a_index]) \
                    .normalized() \
                    .cross(normal)

                other: EdgeConnection = edge_to_face[key]

                if tangent.dot(other.normal) > 0.01:
                    continue

                triangle.mark_edge_unused(index)
                other.triangle.mark_edge_unused(other.edge_index)

    def write_out(self, file, force_subdivisions: mathutils.Vector | None = None):
        file.write('CMSH'.encode())
        kd_tree = KdMeshIndex(self.vertices, self.triangles)
        kd_tree.serialize(file)

    def is_empty(self):
        return len(self.triangles) == 0
    
    def copy_blank(self):
        return MeshCollider()
    
    def append_vertex(self, vertex_data):
        self.vertices.append(vertex_data)

    def append_triangle(self, a, b, c):
        self.triangles.append(MeshColliderTriangle([a, b, c]))

    def get_triangles(self):
        result: list[list[int]] = []

        for triangle in self.triangles:
            result.append(triangle.indices)

        return result
    
    def get_vertex(self, index):
        return self.vertices[index]
    
    def get_vertex_interpolated(self, a, b, lerp):
        lerp_inv = 1 - lerp
        return self.vertices[a] * lerp_inv + self.vertices[b] * lerp