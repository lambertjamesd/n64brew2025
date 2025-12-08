import mathutils
from . import mesh

plane_split_tolerance = 1 / 32

def _did_cross(a: str, b: str) -> bool:
    return a == 'front' and b == 'back' or a == 'back' and b == 'front'

def split_on_side(input: mesh.mesh_data, into_mesh: mesh.mesh_data, on_side: str, index_mapping: dict, triangle: list[int], index_sides: list[str], plane_distance: list[float]):   
    if not (on_side in index_sides):
        return False

    new_index_loop = []

    for i, side in enumerate(index_sides):
        point_index = triangle[i]

        if side == on_side or side == 'both':
            if point_index in index_mapping:
                new_index_loop.append(index_mapping[point_index])
            else:
                next_vertex_index = len(into_mesh.vertices)
                index_mapping[point_index] = next_vertex_index
                into_mesh.append_vertex(input.get_vertex(point_index))
                new_index_loop.append(next_vertex_index)

        next_side = index_sides[(i + 1) % 3]

        if _did_cross(side, next_side):
            next_point_index = triangle[(i + 1) % 3];
            next_distance = plane_distance[next_point_index]
            curr_distance = plane_distance[point_index]

            lerp_value = -curr_distance / (next_distance - curr_distance)

            next_vertex_index = len(into_mesh.vertices)
            new_index_loop.append(next_vertex_index)

            interpolated = input.get_vertex_interpolated(
                point_index,
                next_point_index,
                lerp_value
            )

            into_mesh.append_vertex(interpolated)

    for i in range(1, len(new_index_loop) - 1):
        into_mesh.append_triangle(
            new_index_loop[0],
            new_index_loop[i],
            new_index_loop[i + 1]
        )

    return True


def split(input: mesh.mesh_data, normal: mathutils.Vector, d: float) -> tuple[mesh.mesh_data | None, mesh.mesh_data | None]:
    back: mesh.mesh_data = input.copy_blank()
    front: mesh.mesh_data = input.copy_blank()

    plane_distance = list(map(lambda x: normal.dot(x) + d, input.vertices))

    def get_index_side(index: int):
        distance = plane_distance[index]

        if distance > plane_split_tolerance:
            return 'front'
        
        if distance < -plane_split_tolerance:
            return 'back'
        
        return 'both'

    front_index_mapping = dict()
    back_index_mapping = dict()

    for triangle in input.get_triangles():
        index_sides = list(map(get_index_side, triangle))

        has_front = split_on_side(input, front, 'front', front_index_mapping, triangle, index_sides, plane_distance)
        has_back = split_on_side(input, back, 'back', back_index_mapping, triangle, index_sides, plane_distance)

        if not has_front and not has_back:
            # the entire polygon was coplanar with the splitting axis
            split_on_side(input, front, 'both', front_index_mapping, triangle, index_sides, plane_distance)
            
    if back.is_empty():
        back = None

    if front.is_empty():
        front = None

    return back, front