
import bpy
from bpy.path import abspath
import os.path
import json
from ..parse.struct_parse import find_structs, find_enums
from ..entities import entry_point
from ..cutscene import parser

int_types = {'i8', 'i16', 'i32'}

class Definitions:
    def __init__(self):
        self.definitions = None
        self.enums = None
        self.loaded_path = None
        self.materials = []
        self.repo_path = None
        self.objects = None
        self.scripts = []
        self.entry_points = []
        self.boolean_variables = []
        self.integer_variables = []

        self._objects_for_library_path = {}

    def _search_blend_files(self, start_path):
        result = []
        curr_path = abspath("//")

        for dirpath, dirnames, filenames in os.walk(start_path):
            for filename in filenames:
                if not filename.endswith('.blend'):
                    continue

                abs_path = os.path.join(dirpath, filename)

                if abs_path == bpy.data.filepath:
                    continue

                relative_path = os.path.relpath(abs_path, curr_path)

                result.append(relative_path)

        return result


    def _search_scripts(self, start_path):
        result = []

        for dirpath, dirnames, filenames in os.walk(start_path):
            for filename in filenames:
                if not filename.endswith('.script'):
                    continue

                abs_path = os.path.join(dirpath, filename)

                if abs_path == bpy.data.filepath:
                    continue

                relative_path = os.path.relpath(abs_path, start_path).replace("\\", "/")

                result.append(f"rom:/{relative_path}")

        return result

    def _search_entry_points(self, start_path):
        siblings = self._search_blend_files(start_path)

        result = []
        base_path = abspath('//')

        for sibling in siblings:
            relative_path = os.path.relpath(os.path.join(base_path, sibling), start_path).replace('.blend', '.scene')

            with bpy.data.libraries.load("//" + sibling, link=True) as (data_from, data_to):
                for obj_name in data_from.objects:
                    if not obj_name.startswith(entry_point.ENTRY_PREFIX):
                        continue
                    
                    result.append(f"rom:/{relative_path}#{obj_name[len(entry_point.ENTRY_PREFIX):]}")

        relative_path = os.path.relpath(bpy.data.filepath, start_path).replace('.blend', '.scene')

        for obj_name in bpy.data.objects.keys():
            if not obj_name.startswith(entry_point.ENTRY_PREFIX):
                continue
            result.append(f"rom:/{relative_path}#{obj_name[len(entry_point.ENTRY_PREFIX):]}")


        result.sort()

        return result
    
    def _load_variables(self, repo_path):
        globals_location = os.path.join(repo_path, 'assets/scripts/globals.script')
        scene_location = bpy.data.filepath[:-len('.blend')] + '.script'

        booleans = ['disconnected']
        integer_variables = ['disconnected']

        try:
            with open(globals_location) as global_file:
                parse_tree = parser.parse(global_file.read(), globals_location)

                for var in parse_tree.globals:
                    if var.type.name.value == 'bool':
                        booleans.append(f"global {var.name.value}: bool")
                    if var.type.name.value in int_types:
                        integer_variables.append(f"global {var.name.value}: {var.type.name.value}")
        except Exception as e:
            print(f"failed to load global {e}")

        if os.path.exists(scene_location):
            try:
                with open(scene_location) as scene_file:
                    parse_tree = parser.parse(scene_file.read(), scene_location)

                    for var in parse_tree.scene_vars:
                        if var.type.name.value == 'bool':
                            booleans.append(f"scene {var.name.value}: bool")
                        if var.type.name.value in int_types:
                            integer_variables.append(f"scene {var.name.value}: {var.type.name.value}")
            except Exception as e:
                print(f"failed to load scene script {e}")

        self.boolean_variables = booleans
        self.integer_variables = integer_variables

    def load(self):
        current_path = bpy.data.filepath

        print('current_path', current_path, self.loaded_path)

        if current_path == self.loaded_path:
            return
        
        repo_path = self._find_repo_path()

        print("reloading from ", self.loaded_path, " to ", current_path)

        self.loaded_path = current_path
        self.repo_path = repo_path

        if not repo_path:
            print("No repo path found")
            return

        self._objects_for_library_path = {}
        
        with open(os.path.join(repo_path, "assets/game_objects.json"), "r") as file:
            self.objects = json.load(file)

        for obj in self.objects["objects"]:
            library_path_parts = obj['mesh'].split('#', 1)

            if library_path_parts[0] != "camera":
                library_path = os.path.join(repo_path, "assets", library_path_parts[0])
                relative_path = '//' + os.path.relpath(library_path, os.path.dirname(bpy.data.filepath))
                obj['library'] = relative_path
                obj['mesh_name'] = library_path_parts[1]

            self._objects_for_library_path[relative_path] = obj
            self._objects_for_library_path[obj['type']] = obj


        print("Found repo path at " + str(repo_path))

        scene_config_path = os.path.join(repo_path, 'src/scene/scene_definition.h')

        if not scene_config_path:
            return
        
        with open(scene_config_path, 'r') as file:
            file_contents = file.read()
            self.definitions = find_structs(file_contents)
            self.enums = find_enums(file_contents)

            print("Found definitions " , self.definitions.keys())

        self.materials = list(filter(lambda x: x.endswith('.mat.blend'), self._search_blend_files(os.path.join(repo_path, 'assets/materials'))))

        print("searching for scripts")
        self.scripts = self._search_scripts(os.path.join(repo_path, 'assets'))
        print("searching for entry points")
        self.entry_points = self._search_entry_points(os.path.join(repo_path, 'assets'))
        print("loading variables")
        self._load_variables(repo_path)
        print("finished")
        


    def _find_repo_path(self):
        curr = abspath("//")

        while not os.path.exists(os.path.join(curr, 'src/scene/scene_definition.h')):
            next = os.path.dirname(curr)

            if next == curr:
                return None
            else:
                curr = next
        
        return curr
    
    def needs_reload(self):
        return bpy.data.filepath != self.loaded_path

    def get_structure_type(self, name):
        def_type_name = f"{name}_definition" 

        if self.needs_reload():
            self.load()

        if not self.definitions:
            return None

        if not def_type_name in self.definitions:
            return None
        
        return self.definitions[def_type_name]
    
    def get_materials(self):
        self.load()
        return self.materials
    
    def get_repo_path(self):
        self.load()
        return self.repo_path
    
    def get_objects_list(self):
        self.load()
        return self.objects["objects"]
    
    def get_boolean_variables(self):
        self.load()
        return self.boolean_variables
        
    def get_integer_variables(self):
        self.load()
        return self.integer_variables

    def get_object_for_library_path(self, path):
        self.load()

        if not path.startswith('//'):
            path = '//' + os.path.relpath(path, os.path.dirname(bpy.data.filepath))

        if path in self._objects_for_library_path:
            return self._objects_for_library_path[path]

        return None
    
    def get_object_for_type(self, type):
        if type in self._objects_for_library_path:
            return self._objects_for_library_path[type]
        return None
    
    def get_struct_info(self, type):
        self.load()

        key = type + '_definition'

        if key in self.definitions:
            return self.definitions[key]
        
        return None
    
    def get_enum(self, enum_name):
        self.load()

        if enum_name in self.enums:
            return self.enums[enum_name]
        
        return None
    
    def get_scripts(self):
        self.load()
        return self.scripts
    
    def get_entry_points(self):
        self.load()
        return self.entry_points
    
    def reload(self):
        self.loaded_path = None


object_definitions = Definitions()