import mathutils
import bpy
import io
import struct
import time
import math
from . import mesh
from . import bounding_box
from . import mesh_split
from . import tiny3d_mesh_writer
from . import mesh_collider
from . import export_settings
from . import material_extract
from . import filename
from . import entities
from . import particles

def subdivide_mesh_list(meshes: list[mesh.mesh_data], normal: mathutils.Vector, start_pos: float, distance_step: float, subdivisions: int) -> list[list[mesh.mesh_data]]:
    result = []
    distance = -(distance_step + start_pos)

    for i in range(subdivisions - 1):
        chunk = []
        next = []

        for mesh in meshes:
            behind, front = mesh_split.split(mesh, normal, distance)

            if behind:
                chunk.append(behind)

            if front:
                next.append(front)
        
        meshes = next
        distance -= distance_step
        result.append(chunk)

    result.append(meshes)

    return result
    
max_block_height = 32767 / 64

class OverworldCell():
    def __init__(self, mesh_data: bytes, y_offset: float):
        self.mesh_data: bytes = mesh_data
        self.y_offset: float = y_offset

    def __str__(self):
        return f"mesh_data(len) {len(self.mesh_data)} y_offset {self.y_offset}"
    
coordinate_convert = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')
coordinate_convert_invert = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

class OverworldDetail():
    def __init__(self, obj: bpy.types.Object):
        self.obj: bpy.types.Object = obj
        self.final_transform: mathutils.Matrix = coordinate_convert_invert @ obj.matrix_world @ coordinate_convert
        loc, rot, scale = self.final_transform.decompose()
        self.position: mathutils.Vector = loc
        self.mesh_filename = filename.rom_filename(obj.data.library.filepath) + '.tmesh'
        

def subdivide_detail_list(detail_list: list, normal: mathutils.Vector, start_pos: float, distance_step: float, subdivisions: int) -> list[list]:
    result: list[list] = []

    for i in range(subdivisions):
        result.append([])

    for detail in detail_list:
        bin_index = math.floor((normal.dot(detail.position) - start_pos) / distance_step)
        bin_index = max(bin_index, 0)
        bin_index = min(bin_index, subdivisions - 1)

        result[bin_index].append(detail)

    return result

def generate_overworld_tile(
        cell: list[mesh.mesh_data], 
        details: list[OverworldDetail], 
        particle_list: list[particles.Particles], 
        side_length: float, 
        x: int, z: int, 
        map_min: mathutils.Vector, 
        settings: export_settings.ExportSettings):
    cell_bb = cell[0].bounding_box()

    for i in range(1, len(cell)):
        cell_bb = bounding_box.union(cell_bb, cell[i].bounding_box())

    mesh_names = list(set(map(lambda x: x.mesh_filename, details)))

    height = cell_bb[1].y - cell_bb[0].y

    vertical_subdivisions = math.ceil(height / side_length)

    y_cells = subdivide_mesh_list(
        cell, 
        mathutils.Vector((0, 1, 0)), 
        cell_bb[0].y, 
        side_length,
        vertical_subdivisions
    )

    detail_cells = subdivide_detail_list(details, mathutils.Vector((0, 1, 0)), cell_bb[0].y, side_length, vertical_subdivisions)
    data = io.BytesIO()
    data.write(struct.pack('>BHH', len(y_cells), len(mesh_names), len(details)))
    data.write(struct.pack('>f', cell_bb[0].y))

    for name in mesh_names:
        name_bytes = name.encode()
        data.write(struct.pack('>B', len(name_bytes)))
        data.write(name_bytes)

    for y, y_cell in enumerate(y_cells):
        cell_corner = mathutils.Vector((
            -(x * side_length + map_min.x), 
            -(y * side_length + cell_bb[0].y), 
            -(z * side_length + map_min.z)
        ))

        for mesh_data in y_cell:
            mesh_data.translate(cell_corner)
            
        tiny3d_mesh_writer.write_mesh(y_cell, None, [], settings, data)

        cell_details = detail_cells[y]
        data.write(struct.pack('>H', len(cell_details)))

        for detail in cell_details:
            data.write(struct.pack('>H', mesh_names.index(detail.mesh_filename)))
            pos, rot, scale = detail.final_transform.decompose()
            adjusted_pos = (pos + cell_corner) * settings.fixed_point_scale

            data.write(struct.pack('>fff', adjusted_pos.x, adjusted_pos.y, adjusted_pos.z))
            data.write(struct.pack('>ffff', rot.x, rot.y, rot.z, rot.w))
            data.write(struct.pack('>fff', scale.x, scale.y, scale.z))

    particle_list = sorted(particle_list, key = lambda x: x.material.name)

    particles.write_particles(particle_list, data)

    return OverworldCell(data.getvalue(), cell_bb[0].y)

def generate_lod0(lod_0_objects: list[bpy.types.Object], subdivisions: int, settings: export_settings.ExportSettings, base_transform: mathutils.Matrix, file):
    lod_0_start_time = time.perf_counter()

    lod_0_settings = settings.copy()
    lod_0_settings.sort_direction = mathutils.Vector((1, 0, 0))
    lod_0_settings.fog_scale = 1 / subdivisions

    scaled_transform = mathutils.Matrix.Scale(1 / subdivisions, 4) @ base_transform
    center_scale = settings.world_scale

    all_meshes: list[tuple[mesh.mesh_data, int, int, int]] = []

    for obj in lod_0_objects:
        mesh_list = mesh.mesh_list(scaled_transform)
        mesh_list.append(obj)

        center = (scaled_transform @ obj.matrix_world.translation) * center_scale

        digit_prefix_length = 0

        while digit_prefix_length < len(obj.name) and obj.name[digit_prefix_length].isdigit():
            digit_prefix_length += 1

        priority = int(obj.name[0: digit_prefix_length]) if digit_prefix_length > 0 else 0

        all_meshes += map(lambda mesh: (mesh, int(center.x), int(center.z), priority), mesh_list.determine_mesh_data(None))

    file.write(struct.pack('>B', len(all_meshes)))

    for single_mesh in all_meshes:
        file.write(struct.pack('>hhH', single_mesh[1], single_mesh[2], single_mesh[3]))
        lod_0_settings.default_material_name = material_extract.material_romname(single_mesh[0].mat)
        lod_0_settings.default_material = material_extract.load_material_with_name(single_mesh[0].mat)
        tiny3d_mesh_writer.write_mesh([single_mesh[0]], None, [], lod_0_settings, file)

    print(f"lod_0 creation time {time.perf_counter() - lod_0_start_time}")

class OverworldInputData():
    def __init__(self):
        pass

def generate_overworld(
        overworld_filename: str, 
        mesh_list: mesh.mesh_list, 
        lod_0_objects: list[bpy.types.Object], 
        collider: mesh_collider.MeshCollider, 
        detail_list: list[OverworldDetail], 
        entity_list: list[entities.ObjectEntry],
        particles_list: list[particles.Particles], 
        subdivisions: int, 
        settings: export_settings.ExportSettings, 
        base_transform: mathutils.Matrix,
        enums: dict,
        variable_context
        ):
    mesh_entries = mesh_list.determine_mesh_data()

    mesh_bb = None

    for entry in mesh_entries:
        entry_bb = entry.bounding_box()

        if mesh_bb:
            mesh_bb = bounding_box.union(mesh_bb, entry_bb)
        else:
            mesh_bb = entry_bb

    width = mesh_bb[1].x - mesh_bb[0].x
    height = mesh_bb[1].z - mesh_bb[0].z

    side_length = max(width, height) / subdivisions

    for detail in detail_list:
        if 'collider' in detail.obj.data:
            collider.append(detail.obj.data['collider'], coordinate_convert_invert @ detail.obj.matrix_world)

    subdivide_time_start = time.perf_counter()
    columns = subdivide_mesh_list(mesh_entries, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    cells = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), columns))
    print(f"subdivide_mesh_list for mesh data {time.perf_counter() - subdivide_time_start}")

    subivide_collider_start = time.perf_counter()
    collider_columns = subdivide_mesh_list([collider], mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    collider_cells: list[list[list[mesh_collider.MeshCollider]]] = list(map(lambda column: subdivide_mesh_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), collider_columns))
    print(f"subdivide_mesh_list for collider data {time.perf_counter() - subivide_collider_start}")

    detail_columns = subdivide_detail_list(detail_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    detail_cells: list[list[list[OverworldDetail]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), detail_columns))

    entity_columns = subdivide_detail_list(entity_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    entity_cells: list[list[list[entities.ObjectEntry]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), entity_columns))

    particle_columns = subdivide_detail_list(particles_list, mathutils.Vector((0, 0, 1)), mesh_bb[0].z, side_length, subdivisions)
    particle_cells: list[list[list[particles.Particles]]] = list(map(lambda column: subdivide_detail_list(column, mathutils.Vector((1, 0, 0)), mesh_bb[0].x, side_length, subdivisions), particle_columns))

    cell_data: list[OverworldCell] = []
    collider_cell_data: list[bytes] = []

    write_mesh_time = 0
    write_collider_time = 0
    first_spawn_id = 0
    
    for z, row in enumerate(cells):
        for x, cell in enumerate(row):
            write_mesh_time -= time.perf_counter()
            cell_data.append(generate_overworld_tile(
                cell,
                detail_cells[z][x],
                particle_cells[z][x],
                side_length,
                x,
                z,
                mesh_bb[0],
                settings
            ))
            write_mesh_time += time.perf_counter()

            write_collider_time -= time.perf_counter()
            collider_data = io.BytesIO()
            if len(collider_cells[z][x]):
                collider_cells[z][x][0].find_needed_edges()
                collider_cells[z][x][0].write_out(collider_data, force_subdivisions = mathutils.Vector((8, 1, 8)))
            else:
                tmp = mesh_collider.MeshCollider()
                tmp.write_out(collider_data, force_subdivisions=mathutils.Vector((8, 1, 8)))

            min_cell_min = mathutils.Vector((
                x * side_length + mesh_bb[0].x,
                mesh_bb[0].x,
                z * side_length + mesh_bb[0].z,
            ))
            
            entities.write_object_groups(
                min_cell_min, min_cell_min + mathutils.Vector((side_length, 0, side_length)),
                entity_cells[z][x],
                enums,
                variable_context,
                first_spawn_id,
                collider_data
            )

            first_spawn_id += len(entity_cells[z][x])

            collider_cell_data.append(collider_data.getvalue())
            write_collider_time += time.perf_counter()

    print(f"write_mesh_time = {write_mesh_time}")
    print(f"write_collider_time = {write_collider_time}")

    with open(overworld_filename, 'wb') as file:
        file.write('OVWD'.encode())

        file.write(struct.pack('>HH', subdivisions, subdivisions))
        file.write(struct.pack('>ff', mesh_bb[0].x, mesh_bb[0].z))
        file.write(struct.pack('>f', side_length))

        generate_lod0(lod_0_objects, subdivisions, settings, base_transform, file)

        visual_block_location = file.tell() + 8 * subdivisions * subdivisions

        actor_block_location = visual_block_location + sum(map(lambda x: len(x.mesh_data), cell_data))

        for index, cell in enumerate(cell_data):
            file.write(struct.pack('>II', visual_block_location, actor_block_location))
            visual_block_location += len(cell.mesh_data)
            actor_block_location += len(collider_cell_data[index])

        for cell in cell_data:
            file.write(cell.mesh_data)

        for actor_cell in collider_cell_data:
            file.write(actor_cell)