import os
import bpy
import json
import sys

base_path = os.path.join(os.getcwd(), 'assets/meshes')

for dirpath, dirnames, filenames in os.walk(base_path):
    for filename in filenames:
        if not filename.endswith('.blend'):
            continue

        full_path = os.path.join(dirpath, filename)
        print("checking " + full_path)
        with bpy.data.libraries.load("//" + full_path, link=True) as (data_from, data_to):
            print("importing ", data_from.meshes)
            data_to.meshes = data_from.meshes

game_objects = {"objects": []}

print('loading existing objects', sys.argv[-1])
with open(sys.argv[-1], 'r') as file:
    game_objects = json.load(file)

object_list = game_objects["objects"]

for mesh in bpy.data.meshes:
    if not mesh.library:
        continue

    if not 'type' in mesh:
        continue

    short_path = os.path.relpath(mesh.library.filepath, base_path)
    relative_path = os.path.join("meshes", short_path) + "#" + mesh.name

    existing_entry = next(filter(lambda x: x["mesh"] == relative_path, object_list), None)

    if existing_entry:
        print("already has ", relative_path)
        existing_entry['type'] = mesh['type']
        continue

    print("adding new ", relative_path)
    object_list.append({
        "name": short_path.removesuffix(".blend").replace("/", "_"),
        "description": "",
        "mesh": relative_path,
        "type": mesh['type'],
    })

object_list.sort(key=lambda x: x["name"])

with open(sys.argv[-1], 'w') as file:
    json.dump(game_objects, file, indent=4)