import bpy
import mathutils
import sys
import io
import os
import math
import struct
import bmesh

sys.path.append(os.path.dirname(__file__))

# FOV = 2 * arctan(sensor_size / (2 * focal_length))
SENSOR_SIZE = 36

import entities.tiny3d_mesh_writer
import entities.mesh
import entities.export_settings
import cutscene.variable_layout

base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
base_transform_inv = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')

def write_static(static_entities: list, settings: entities.export_settings.ExportSettings, file: io.BufferedWriter):    
    mesh_list: entities.mesh.mesh_list = entities.mesh.mesh_list(base_transform)

    for entity in static_entities:
        if len(entity.data.materials) > 0:
            mesh_list.append(entity)

    
    mesh_data = mesh_list.determine_mesh_data()
    entities.tiny3d_mesh_writer.write_mesh(mesh_data, None, [], settings, file)

def write_raycast_mesh(mesh: bpy.types.Mesh, file: io.BufferedWriter):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])

    file.write(struct.pack('>HH', len(bm.verts), len(bm.faces)))

    for vert in bm.verts:
        rotated = base_transform @ vert.co
        file.write(struct.pack('>fff', rotated.x, rotated.y, rotated.z))

    for face in bm.faces:
        file.write(struct.pack('>HHH', face.verts[0].index, face.verts[1].index, face.verts[2].index))

    bm.free()

def write_boolean(boolean_enum: dict[str, int], name: str, file: io.BufferedWriter):
    if name in boolean_enum:
        file.write(boolean_enum[name].to_bytes(2, 'big'))
    else:
        file.write(boolean_enum['disconnected'].to_bytes(2, 'big'))
    
def write_parts(parts: dict, part_starts: dict, boolean_enum: dict[str, int], settings: entities.export_settings.ExportSettings, file: io.BufferedWriter):
    file.write(len(parts).to_bytes(2, 'big'))

    for name, obj in parts.items():
        if not name in part_starts:
            print(f"missing staring point for {name}")
            sys.exit(1)

        start = part_starts[name]
        start_transform = base_transform @ start.matrix_world @ base_transform_inv
        loc, rot, scale = start_transform.decompose()
        file.write(struct.pack('>fff', loc.x, loc.y, loc.z))
        file.write(struct.pack('>ffff', rot.x, rot.y, rot.z, rot.w))

        end_transform = base_transform @ obj.matrix_world @ base_transform_inv
        loc, rot, scale = end_transform.decompose()
        file.write(struct.pack('>fff', loc.x, loc.y, loc.z))
        file.write(struct.pack('>ffff', rot.x, rot.y, rot.z, rot.w))

        mesh_list: entities.mesh.mesh_list = entities.mesh.mesh_list(base_transform @ obj.matrix_world.inverted())
        mesh_list.append(obj)
        mesh_data = mesh_list.determine_mesh_data()
        entities.tiny3d_mesh_writer.write_mesh(mesh_data, None, [], settings, file)

        write_raycast_mesh(obj.data, file)
        write_boolean(boolean_enum, obj['has_part'], file)

def write_camera(camera, file: io.BufferedWriter):
    camera_transform = base_transform @ camera.matrix_world
    loc, rot, scale = camera_transform.decompose()
    file.write(struct.pack('>fff', loc.x, loc.y, loc.z))
    file.write(struct.pack('>ffff', rot.x, rot.y, rot.z, rot.w))
    file.write(struct.pack('>f', camera.data.angle_y))


def write_scene_vars(repair_scene, boolean_enum: dict[str, int], file: io.BufferedWriter):
    write_boolean(boolean_enum, repair_scene['puzzle_complete'], file)
    str_bytes = repair_scene['exit_scene'].encode()
    file.write(len(str_bytes).to_bytes(1, 'big'))
    file.write(str_bytes)

def process_scene():
    camera = None
    repair_scene = None
    parts = {}
    part_starts = {}
    static_entities = []
    settings = entities.export_settings.ExportSettings()
    settings.fixed_point_scale = 128
    output_filename = sys.argv[-1]

    for obj in bpy.data.objects:
        if obj.type == 'CAMERA':
            camera = obj
            continue

        if obj.type != 'MESH':
            continue

        if 'type' in obj.data:
            if obj.data['type'] == 'repair_scene':
                repair_scene = obj
            continue

        if not obj.name.startswith('part_'):
            static_entities.append(obj)
            continue

        if obj.name.endswith('_start'):
            part_starts[obj.name[len('part_'):-len('_start')]] = obj
        else:
            parts[obj.name[len('part_'):]] = obj

    if not camera:
        print('The scene is missing a camera')
        sys.exit(1)

    if not repair_scene:
        print('The scene is missing the repair scene object')
        sys.exit(1)

    globals = cutscene.variable_layout.VariableLayout()

    with open('build/assets/scripts/globals.json') as file:
        globals.deserialize(file)

    scene = cutscene.variable_layout.VariableLayout()

    boolean_enum, integer_enum, entity_id_enum = cutscene.variable_layout.build_variables(globals, scene)

    print(f"found parts {', '.join(parts.keys())}")
    print(f"static count {len(static_entities)}")

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())
        write_static(static_entities, settings, file)
        write_parts(parts, part_starts, boolean_enum, settings, file)
        write_camera(camera, file)
        write_scene_vars(repair_scene, boolean_enum, file)

process_scene()