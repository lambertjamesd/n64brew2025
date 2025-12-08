import bpy
import sys
import os.path

sys.path.append(os.path.dirname(__file__))

import entities.material_extract
import entities.serialize

if __name__ == "__main__":
    output_directory = sys.argv[-1]

    for material in bpy.data.materials:
        if not 'f3d_mat' in material or not 'rdp_settings' in material['f3d_mat']:
            continue
        
        if output_directory.endswith('.mat'):
            output_filename= output_directory
        else:
            output_filename = os.path.join(output_directory, material.name) + '.mat'

        print(f'Writing material to {output_filename}')
        result = entities.material_extract.determine_material_from_f3d(material)
        print(result)
        os.makedirs(os.path.dirname(output_filename), exist_ok=True)
        entities.serialize.serialize_material(output_filename, result)
