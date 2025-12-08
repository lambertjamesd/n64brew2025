
from . import mesh
from . import material
from . import material_delta

class mesh_chunk():
    def __init__(self, data: mesh.mesh_data, used_bones: tuple[int, int, int] | tuple[int, int] | tuple[int], mat: material.Material):
        self.data: mesh.mesh_data = data
        self.used_bones: tuple[int, int, int] | tuple[int, int] | tuple[int] = used_bones
        self.material: material.Material = mat

def remove_duplicates(mesh_data: mesh.mesh_data) -> mesh.mesh_data:
    first_vertex_index = {}
    index_mapping = {}

    result = mesh.mesh_data(mesh_data.mat)

    for idx, vertex in enumerate(mesh_data.vertices):
        key = mesh.pack_vertex(
            vertex,
            mesh_data.uv[idx],
            mesh_data.color[idx],
            mesh_data.normals[idx],
            mesh_data.bone_indices[idx]
        )

        if key in first_vertex_index:
            index_mapping[idx] = first_vertex_index[key]
        else:
            vertex_index = len(first_vertex_index)
            index_mapping[idx] = vertex_index
            first_vertex_index[key] = vertex_index

            result.vertices.append(vertex)
            result.normals.append(mesh_data.normals[idx])
            result.color.append(mesh_data.color[idx])
            result.uv.append(mesh_data.uv[idx])
            result.bone_indices.append(mesh_data.bone_indices[idx])

    for index in mesh_data.indices:
        result.indices.append(index_mapping[index])

    return result

def remove_unused_vertices(mesh_data: mesh.mesh_data) -> mesh.mesh_data:
    indices_set = set(mesh_data.indices)

    index_mapping = {}

    result = mesh.mesh_data(mesh_data.mat)

    for i in range(len(mesh_data.vertices)):
        if i in indices_set:
            index_mapping[i] = len(index_mapping)

            result.vertices.append(mesh_data.vertices[i])
            result.normals.append(mesh_data.normals[i])
            result.color.append(mesh_data.color[i])
            result.uv.append(mesh_data.uv[i])
            result.bone_indices.append(mesh_data.bone_indices[i])

    result.indices = [index_mapping[index] for index in mesh_data.indices]

    return result

def split_into_bone_pairs(mesh_data: mesh.mesh_data, mat: material.Material) -> list[mesh_chunk]:
    result: dict[tuple[int, int, int], mesh.mesh_data] = {}

    for idx in range(0, len(mesh_data.indices), 3):
        indices = mesh_data.indices[idx:idx+3]

        bone_indices = [mesh_data.bone_indices[index] for index in indices]

        key = tuple(sorted(list(set(bone_indices))))

        if key in result:
            result[key].indices += indices
        else:
            new_mesh = mesh_data.copy()
            new_mesh.indices = indices
            result[key] = new_mesh

    return [mesh_chunk(remove_unused_vertices(data), key, mat) for key, data in result.items()]

# TODO use tiny3d time
MATRIX_TIME = 4.134

def determine_chunk_order(chunks: list[mesh_chunk], default_material: material.Material) -> list[mesh_chunk]:
    current_material = default_material
    current_bones = None

    result = []

    for render_layer in range(0, 3):
        chunk_layer = list(filter(lambda x: x.material.get_render_layer() == render_layer, chunks))
        target_length = len(result) + len(chunk_layer)
        used_chunks = set()

        while len(result) < target_length:
            next_chunk_index = None
            next_cost = 0

            for idx, chunk in enumerate(chunk_layer):
                if idx in used_chunks:
                    continue

                diff = material_delta.determine_material_delta(current_material, chunk.material)

                cost_to_switch = material_delta.determine_material_cost(diff)

                if current_bones != chunk.used_bones:
                    cost_to_switch += MATRIX_TIME

                if next_chunk_index == None or cost_to_switch < next_cost:
                    next_chunk_index = idx
                    next_cost = cost_to_switch

            next_chunk: mesh_chunk = chunk_layer[next_chunk_index]
            current_bones = next_chunk.used_bones
            current_material = next_chunk.material
            result.append(next_chunk)
            used_chunks.add(next_chunk_index)

    return result

def chunkify_mesh(mesh_data: mesh.mesh_data, mesh_material: material.Material, default_material: material.Material) -> list[mesh_chunk]:
    return split_into_bone_pairs(mesh_data, mesh_material)