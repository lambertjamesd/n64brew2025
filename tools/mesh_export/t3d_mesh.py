import bpy
import mathutils
import bmesh
import sys
import math
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.mesh
import entities.tiny3d_mesh_writer
import entities.export_settings
import entities.mesh_optimizer
import entities.material_extract
import entities.animation
import entities.armature

def replace_extension(filename: str, ext: str) -> str:
    return os.path.splitext(filename)[0]+ext

APPLIED_MODIFIERS = {
    "MIRROR",
    "SIMPLE_DEFORM",
    "SHRINKWRAP",
}

def process_scene():
    bpy.ops.object.mode_set(mode="OBJECT")

    base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

    arm: entities.armature.ArmatureData | None = None

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if not obj.name in bpy.context.view_layer.objects:
            continue

        bpy.context.view_layer.objects.active = obj

        for modifier in obj.modifiers:
            if not (modifier.type in APPLIED_MODIFIERS):
                print("skipping modifier " + modifier.type)
                continue
            bpy.ops.object.modifier_apply(modifier=modifier.name, single_user = True)


    for obj in bpy.data.objects:
        if obj.type != 'ARMATURE':
            continue

        if not arm is None:
            raise Exception('Only 1 armature allowed')

        arm = entities.armature.ArmatureData(obj, base_transform)

    if arm:
        for obj in bpy.data.objects:
            arm.check_connected_mesh_object(obj)

        arm.build_bone_data()

    for mesh in bpy.data.meshes:
        bm = bmesh.new()
        bm.from_mesh(mesh)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(mesh)
        bm.free()

    mesh_list = entities.mesh.mesh_list(base_transform)
    attatchments: list[entities.armature.BoneLinkage] = []

    use_scene = None

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        if obj.data.library:
            if obj.parent and obj.parent_bone and arm:
                attatchments.append(arm.find_bone_linkage(obj))
            
            continue

        mesh_list.append(obj)
        use_scene = obj.users_scene[0]

    settings = entities.export_settings.ExportSettings()
    settings.fixed_point_scale = 128

    meshes = mesh_list.determine_mesh_data(arm)

    meshes = list(map(lambda x: entities.mesh_optimizer.remove_duplicates(x), meshes))

    if 'default_material' in use_scene and use_scene['default_material']:
        default_material = use_scene['default_material']
        settings.default_material_name = entities.material_extract.material_romname(default_material)
        settings.default_material = entities.material_extract.load_material_with_name(default_material)
    elif len(meshes) == 1 and entities.material_extract.material_can_extract(meshes[0].mat):
        settings.default_material_name = entities.material_extract.material_romname(meshes[0].mat)
        settings.default_material = entities.material_extract.load_material_with_name(meshes[0].mat)

    if 'light_source' in use_scene:
        settings.light_source = use_scene['light_source']

    with open(sys.argv[-1], 'wb') as file:
        entities.tiny3d_mesh_writer.write_mesh(meshes, arm, attatchments, settings, file)

    entities.animation.export_animations(replace_extension(sys.argv[-1], '.anim'), arm, settings)
    
process_scene()