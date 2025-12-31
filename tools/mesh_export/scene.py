import bpy
import sys
import os
import mathutils
import math
import struct
import io

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.mesh_collider
import entities.tiny3d_mesh_writer
import entities.mesh_optimizer
import entities.material_extract
import entities.entry_point
import entities.particles
from entities.entities import ObjectEntry
import entities.overworld
import parse.struct_parse
import parse.struct_serialize
import cutscene.expresion_generator
import cutscene.parser
import cutscene.tokenizer
import cutscene.variable_layout
import entities.camera_animation
import entities.room

class StaticEntry():
    def __init__(self, obj: bpy.types.Object, mesh: bpy.types.Mesh, transform: mathutils.Matrix):
        self.obj = obj
        self.mesh = mesh

class ParticlesEntry():
    def __init__(self, obj: bpy.types.Object):
        self.obj = obj

class LocationEntry():
    def __init__(self, obj: bpy.types.Object, name: str):
        self.obj: bpy.types.Object = obj
        self.name: str = name
        self.on_enter: str = obj['on_enter'] if 'on_enter' in obj else ''
        self.room_index = 0

class LoadingZone():
    def __init__(self, obj: bpy.types.Object, target: str):
        self.obj: bpy.types.Object = obj
        self.target: str = target

    def bounding_box(self, scene_rotation: mathutils.Matrix) -> tuple[mathutils.Vector, mathutils.Vector]:
        final_transform = scene_rotation @ self.obj.matrix_world
        transformed = [final_transform @ mathutils.Vector(vtx) for vtx in self.obj.bound_box]

        bb_min = transformed[0]
        bb_max = transformed[0]

        for vtx in transformed:
            bb_min = mathutils.Vector((
                min(bb_min.x, vtx.x),
                min(bb_min.y, vtx.y),
                min(bb_min.z, vtx.z)
            ))
            bb_max = mathutils.Vector((
                max(bb_max.x, vtx.x),
                max(bb_max.y, vtx.y),
                max(bb_max.z, vtx.z)
            ))

        return bb_min, bb_max

class Scene():
    def __init__(self):
        self.static:list[StaticEntry] = []
        self.objects: list[ObjectEntry] = []
        self.particles: list[ParticlesEntry] = []
        self.locations: list[LocationEntry] = []
        self.loading_zones: list[LoadingZone] = []
        self.scene_mesh_collider = entities.mesh_collider.MeshCollider()

def write_string(value: str, file):
    byte_encoded = value.encode()
    file.write(struct.pack('>B', len(byte_encoded)))
    file.write(byte_encoded)

def get_object_type(obj: bpy.types.Object) -> str:
    if 'type' in obj:
        return obj['type']
    
    if obj.data != None and 'type' in obj.data:
        return obj.data['type']
    
    return None

def process_linked_object(obj: bpy.types.Object, mesh: bpy.types.Mesh, definitions: dict[str, parse.struct_parse.StructureInfo], room_index: int):
    type = get_object_type(obj)

    if not type:
        return None
    
    def_type_name = f"{type}_definition" 

    if not def_type_name in definitions:
        raise Exception(f"could not find def type {def_type_name}")
    
    print(f"found object {obj.name} of type {def_type_name}")

    return ObjectEntry(obj, type, definitions[def_type_name], room_index)

def write_static(scene: Scene, base_transform: mathutils.Matrix, room_collection: entities.room.room_collection, file):
    settings = entities.export_settings.ExportSettings()

    for entry in scene.static:
        room_collection.get_obj_room_index(entry.obj)

    mesh_list_for_rooms = []

    for i in range(len(room_collection.rooms)):
        mesh_list_for_rooms.append(entities.mesh.mesh_list(base_transform))

    for entry in scene.static:
        mesh_list_for_rooms[room_collection.get_obj_room_index(entry.obj)].append(entry.obj)

    meshes_for_rooms = list(map(lambda x: x.determine_mesh_data(), mesh_list_for_rooms))

    for i in range(len(meshes_for_rooms)):
        meshes_for_rooms[i] = list(map(lambda x: entities.mesh_optimizer.remove_duplicates(x), meshes_for_rooms[i]))

    meshes = []

    for room_meshes in meshes_for_rooms:
        for mesh in room_meshes:
            meshes.append(mesh)

    file.write(len(meshes).to_bytes(2, 'big'))

    for mesh in meshes:
        # this signals the mesh should be embedded
        file.write(b'\0')

        settings.default_material_name = entities.material_extract.material_romname(mesh.mat)
        settings.default_material = entities.material_extract.load_material_with_name(mesh.mat)

        entities.tiny3d_mesh_writer.write_mesh([mesh], None, [], settings, file)

    room_count = len(meshes_for_rooms)
    file.write(room_count.to_bytes(2, 'big'))
    index_start = 0

    for room_meshes in meshes_for_rooms:
        file.write(struct.pack('>HH', index_start, index_start + len(room_meshes)))
        index_start += len(room_meshes)

def write_particles(scene: Scene, base_transform: mathutils.Matrix, room_collection: entities.room.room_collection, file):
    room_to_particle: list[list[entities.particles.Particles]] = []

    for room in room_collection.rooms:
        room_to_particle.append([])
        
    count = 0

    for particles in scene.particles:
        built = entities.particles.build_particles(particles.obj, base_transform)

        if not built:
            continue

        room_index = room_collection.get_obj_room_index(particles.obj)
        room_to_particle[room_index].append(built)
        count = count + 1

    particle_data = io.BytesIO()

    for room_index in range(len(room_collection.rooms)):
        particles = room_to_particle[room_index]

        for built in particles:
            particle_data.write(built.particles)

    particle_data_bytes = particle_data.getvalue()

    file.write(struct.pack('>IH', len(particle_data_bytes), count))
    file.write(particle_data_bytes)

    for room_index in range(len(room_collection.rooms)):
        particles = room_to_particle[room_index]

        particles = entities.particles.batch_particles(particles)

        for built in particles:
            built.write_into(file)

    offset = 0

    for room_index in range(len(room_collection.rooms)):
        particles = room_to_particle[room_index]
        file.write(struct.pack('>HH', offset, offset + len(particles)))
        offset += len(particles)

def find_static_blacklist():
    result = set()

    for collection in bpy.data.collections:
        if collection.name.startswith('lod_'):
            result = result.union(collection.all_objects)

    return result

def check_for_overworld(base_transform: mathutils.Matrix, overworld_filename: str, definitions, enums, variable_context):
    settings = entities.export_settings.ExportSettings()

    if not ('lod_1' in  bpy.data.collections):
        return False
    
    collection: bpy.types.Collection = bpy.data.collections["lod_1"]

    mesh_list = entities.mesh.mesh_list(base_transform)
    detail_list: list[entities.overworld.OverworldDetail] = []
    entity_list: list[entities.entities.ObjectEntry] = []
    particle_list: list[entities.particles.Particles] = [];

    collider = entities.mesh_collider.MeshCollider()

    for obj in collection.all_objects:
        final_transform = base_transform @ obj.matrix_world

        if obj.type != "MESH":
            continue

        obj_type = get_object_type(obj)

        if obj_type != None:
            if obj_type == 'none':
                continue
            elif obj_type == 'static_particles':
                particles = entities.particles.build_particles(obj, base_transform)

                particle_list.append(particles)
            else:
                entity_list.append(process_linked_object(obj, obj.data, definitions, 0))
            continue

        mesh: bpy.types.Mesh = obj.data

        if mesh.library:
            detail_list.append(entities.overworld.OverworldDetail(obj))
            continue

        if len(mesh.materials) > 0:
            mesh_list.append(obj)

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH':
            collider.append(mesh, final_transform)

    lod_0_collection: bpy.types.Collection | None = bpy.data.collections["lod_0"] if "lod_0" in bpy.data.collections else None
    lod_0_objects = []

    if lod_0_collection:
        lod_0_objects = list(lod_0_collection.all_objects)

    subdivisions = 8
    
    if 'subdivisions' in collection:
        subdivisions = collection['subdivisions']

    entities.overworld.generate_overworld(
        overworld_filename, 
        mesh_list, 
        lod_0_objects, 
        collider, 
        detail_list, 
        entity_list,
        particle_list,
        subdivisions, 
        settings, 
        base_transform,
        enums,
        variable_context
    )

    return True
    
def load_cutscene_vars(input_filename: str, generated_bools, var_json_path):
    scene_vars_builder = cutscene.variable_layout.VariableLayoutBuilder()
    function_names: list[str] = []

    cutscene_filename = input_filename[:-6] + '.script'

    success = True
    exists = os.path.exists(cutscene_filename)

    if exists:
        with open(cutscene_filename) as scene_vars_file:
            scene_vars_parse_tree = cutscene.parser.parse(scene_vars_file.read(), cutscene_filename)

            for var in scene_vars_parse_tree.scene_vars:
                success = scene_vars_builder.add_variable(var) and success

            # main function
            function_names.append("")
            for fn in scene_vars_parse_tree.functions:
                function_names.append(fn.name.value)

        cutscene_filename = f"rom:{cutscene_filename[6:]}"
    else:
        cutscene_filename = None

    for bool_name in generated_bools:
        scene_vars_builder.add_generated_variable(bool_name, "bool")

    if not success:
        raise Exception('failed to load cutscene vars')

    with open(var_json_path, 'w') as file:
        scene_vars_builder.serialize(file)

    return scene_vars_builder.build(), function_names, cutscene_filename

int_types = {'i8': 1, 'i16': 2, 'i32': 3}

TYPE_OFFSET = 13
SCENE_FLAG = 0x8000

def int_type_flag(name: str) -> int:
    return int_types[name] << TYPE_OFFSET

def build_variable_enum(enums: dict, globals: cutscene.variable_layout.VariableLayout, scene: cutscene.variable_layout.VariableLayout):
    boolean_enum, integer_enum, entity_id_enum = cutscene.variable_layout.build_variables(globals, scene)

    enums['boolean_variable'] = parse.struct_parse.UnorderedEnum('boolean_variable', boolean_enum)
    enums['integer_variable'] = parse.struct_parse.UnorderedEnum('integer_variable', integer_enum)
    enums['entity_id_variable'] = parse.struct_parse.UnorderedEnum('entity_id_variable', entity_id_enum)
        

def build_room_entity_block(objects: list[ObjectEntry], variable_context, context, enums: dict[str, parse.struct_parse.UnorderedEnum]) -> bytes:
    block = io.BytesIO()

    block.write(len(objects).to_bytes(2, 'big'))

    entity_enum = enums['entity_id_variable']

    for object in objects:
        object.write_condition(variable_context, block)
        on_despawn = enums['boolean_variable'].str_to_int(object.on_despawn or "disconnected")

        script_location_name = f"scene {object.obj.name}: entity_id"
        
        if not entity_enum.is_defined(script_location_name):
            script_location_name = 'disconnected'

        block.write(struct.pack('>HH', on_despawn, entity_enum.str_to_int(script_location_name)))

        def_type = enums['enum entity_type_id'].str_to_int('ENTITY_TYPE_' + object.name)
        block.write(def_type.to_bytes(2, 'big'))

        struct_size = parse.struct_serialize.obj_gather_types(object.def_type, context)
        block.write(struct_size.to_bytes(2, 'big'))
    
        object.write_definition(context, block)

    return block.getvalue()

def find_scene_objects(scene, definitions, room_collection, base_transform):
    object_blacklist = find_static_blacklist()

    for obj in bpy.data.objects:
        if obj.name.startswith('fast64_f3d_material_library_'):
            continue

        if 'loading_zone' in obj:
            scene.loading_zones.append(LoadingZone(obj, obj['loading_zone']))
            continue

        if entities.entry_point.is_entry_point(obj):
            scene.locations.append(LocationEntry(obj, entities.entry_point.get_entry_point(obj)))
            continue

        if obj in object_blacklist:
            continue

        obj_type = get_object_type(obj)

        if obj_type != None:
            if obj_type == 'none':
                continue
            if obj_type == 'static_particles':
                scene.particles.append(ParticlesEntry(obj))
            else:
                scene.objects.append(process_linked_object(obj, obj.data, definitions, room_collection.get_obj_room_index(obj)))
            continue

        if obj.type != "MESH":
            continue

        final_transform = base_transform @ obj.matrix_world

        mesh: bpy.types.Mesh = obj.data

        if len(mesh.materials) > 0 and not obj.name.startswith('collision'):
            scene.static.append(StaticEntry(obj, mesh, final_transform))

        if obj.rigid_body and obj.rigid_body.collision_shape == 'MESH' or obj.name.startswith('collision'):
            scene.scene_mesh_collider.append(mesh, final_transform)

def write_room_entiites(room_collection, grouped, shared_entity_index, variable_context, context, enums, file):
    for room_index in range(len(room_collection.rooms)):
        if room_index in grouped:
            objects = grouped[room_index]
        else:
            objects = []

        room_block = build_room_entity_block(objects, variable_context, context, enums)

        file.write(struct.pack('>H', len(room_block)))
        file.write(room_block)
        
        if room_index in shared_entity_index:
            indices = shared_entity_index[room_index]
            file.write(struct.pack('>H', len(indices)))
            for index in indices:
                file.write(struct.pack('>H', index))
        else:
            file.write(struct.pack('>H', 0))

def write_shared_entities(shared_entities, variable_context, context, enums, file):
    shared_block = build_room_entity_block(shared_entities, variable_context, context, enums)
    file.write(struct.pack('>HH', len(shared_entities), len(shared_block)))
    file.write(shared_block)

def write_loading_zones(scene, base_transform, context, file):
    file.write(struct.pack(">H", len(scene.loading_zones)))

    for loading_zone in scene.loading_zones:
        bb_min, bb_max = loading_zone.bounding_box(base_transform)
        file.write(struct.pack(">fff", bb_min.x, bb_min.y, bb_min.z))
        file.write(struct.pack(">fff", bb_max.x, bb_max.y, bb_max.z))
        file.write(struct.pack(">I", context.get_string_offset(loading_zone.target)))

def read_room_objects(scene: Scene, scene_vars, context) -> tuple[dict[int, list[ObjectEntry]], dict[int, list[int]], list[ObjectEntry]]:
    grouped: dict[int, list[ObjectEntry]] = {}
    shared_entity_index: dict[int, list[int]] = {}
    shared_entities: list[ObjectEntry] = []

    expected_spawners: set[str] = set([entry.name for entry in scene_vars.get_all_entries() if entry.type_name == "entity_spawner"])

    for object in scene.objects:
        parse.struct_serialize.layout_strings(object.obj, object.def_type, context, None)

        key = object.room_index

        multiroom_ids = object.get_multiroom_ids(context)

        if len(multiroom_ids):
            entity_index = len(shared_entities)

            for room_id in multiroom_ids:
                if room_id in shared_entity_index:
                    shared_entity_index[room_id].append(entity_index)
                else:
                    shared_entity_index[room_id] = [entity_index]

            shared_entities.append(object)
            continue

        object_index = 0

        if key in grouped:
            object_index = len(grouped[key])
            grouped[key].append(object)
        else:
            grouped[key] = [object]

        spawner_var_name = f"{object.obj.name}_spawner"
        var_type = scene_vars.get_variable_type(spawner_var_name)

        if var_type:
            if var_type != "entity_spawner":
                raise Exception(f"the variable {spawner_var_name} should be of type entity_spawner")
            else:
                expected_spawners.remove(spawner_var_name)
                scene_vars.set_initial_value(spawner_var_name, (object.room_index << 16) | object_index)

    if len(expected_spawners) > 0:
        raise Exception(f"expected spawners {','.join(expected_spawners)}. Either remove these variables from the script or add an object to the scene with the spanwer name with the _spawner suffix removed")
    
    return  grouped, shared_entity_index, shared_entities

def process_scene():
    input_filename = sys.argv[1]
    output_filename = sys.argv[-2]
    overworld_filename = sys.argv[-1]

    scene = Scene()

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
    definitions = {}
    enums = {}
    room_collection = entities.room.room_collection()

    room_collection.get_room_index('room_default')

    with open('src/scene/scene_definition.h', 'r') as file:
        file_content = file.read()
        definitions = parse.struct_parse.find_structs(file_content)
        enums = parse.struct_parse.find_enums(file_content)

    room_names = [parse.struct_parse.EnumValue('room_default', 0)]

    for collection in bpy.data.collections:
        if collection.name.startswith('room_'):
            room_names.append(parse.struct_parse.EnumValue(collection.name, len(room_names)))

    room_enum = parse.struct_parse.EnumInfo('enum rooms', room_names)

    for entry in sorted(room_names, key=lambda x: x.name):
        room_collection.get_room_index(entry.name)

    enums['enum rooms'] = room_enum

    globals = cutscene.variable_layout.VariableLayout()

    with open('build/assets/scripts/globals.json') as file:
        globals.deserialize(file)
        
    find_scene_objects(scene, definitions, room_collection, base_transform)

    generated_bools = []

    for obj in scene.objects:
        if not obj.should_auto_gen_condition():
            continue

        variable_name = obj.get_on_despawn()

        if variable_name == None:
            variable_name = f"_auto_bool_{len(generated_bools)}"
            generated_bools.append(variable_name)

        obj.generate_spawn_condition(variable_name)

    scene_vars, function_names, cutscene_filename = load_cutscene_vars(
        input_filename, 
        generated_bools,
        f"{output_filename[:-len('.scene')]}.json"
    )

    build_variable_enum(enums, globals, scene_vars)

    variable_context = cutscene.variable_layout.VariableContext(globals, scene_vars, cutscene.variable_layout.VariableLayout())

    context = parse.struct_serialize.SerializeContext(enums)

    has_overworld = check_for_overworld(base_transform, overworld_filename, definitions, enums, variable_context)

    with open(output_filename, 'wb') as file:
        file.write('WRLD'.encode())

        file.write(len(scene.locations).to_bytes(1, 'big'))

        for location in scene.locations:
            write_string(location.name, file)
            write_string(location.on_enter, file)

            parse.struct_serialize.write_vector3_position(file, location.obj)
            parse.struct_serialize.write_vector2_rotation(file, location.obj)

            file.write(struct.pack('>H', room_collection.get_obj_room_index(location.obj)))

        write_static(scene, base_transform, room_collection, file)
        write_particles(scene, base_transform, room_collection, file)

        scene.scene_mesh_collider.find_needed_edges()
        scene.scene_mesh_collider.write_out(file)

        for loading_zone in scene.loading_zones:
            context.get_string_offset(loading_zone.target)

        grouped = {}
        shared_entity_index = {}
        shared_entities = []

        if not has_overworld:
            grouped, shared_entity_index, shared_entities = read_room_objects(scene, scene_vars, context)

        context.write_strings(file)

        write_room_entiites(room_collection, grouped, shared_entity_index, variable_context, context, enums, file)

        write_shared_entities(shared_entities, variable_context, context, enums, file)

        write_loading_zones(scene, base_transform, context, file)

        if has_overworld:
            overworld_file_location = f"rom:/{overworld_filename[len('filesystem/'):]}"
            write_string(overworld_file_location, file)
        else:
            file.write(b'\0')

        entities.camera_animation.export_camera_animations(output_filename.replace('.scene', '.sanim'), file)

        if cutscene_filename:
            write_string(cutscene_filename, file)
        else:
            file.write(b'\0')
        for room in room_collection.rooms:
            fn_index = function_names.index(room) if room in function_names else 0xFFFF
            file.write(fn_index.to_bytes(2, 'big'))
        scene_vars.write_default_values(file)
            

process_scene()