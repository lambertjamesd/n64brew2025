import bpy
from .definitions import object_definitions
import os
from ..entities import entry_point
from ..parse import struct_parse
import bmesh
import mathutils

def _get_item_types(self, context):
    return list(map(lambda x: (x.name, x.name, ''), object_definitions.get_enum('enum inventory_item_type').all_values()))

def _get_scripts(self, context):
    return list(map(lambda x: (x, x, ''), object_definitions.get_scripts()))

def _get_rooms(self, context):
    result = []

    for collection in bpy.data.collections:
        if collection.name.startswith('room_'):
            result.append(collection.name)

    return list(map(lambda x: (x, x, ''), result))

def _get_booleans(self, context):
    return list(map(lambda x: (x, x, ''), object_definitions.get_boolean_variables()))
    
def _get_integers(self, context):
    return list(map(lambda x: (x, x, ''), object_definitions.get_integer_variables()))

def _get_entry_points(self, context):
    return list(map(lambda x: (x, x[len('rom:/scenes/'):], ''), object_definitions.get_entry_points()))

def _get_positions(self, context):
    result = []

    for obj in bpy.data.objects:
        result.append(f'obj {obj.name}')

    return list(map(lambda x: (x, x, ''), result))

def _get_objects(self, context):
    object_definitions.load()
    result = []
    for game_object_def in object_definitions.get_objects_list():
        result.append((game_object_def["name"], game_object_def["name"], game_object_def["description"]))
    return result

class NODE_OT_game_object_item_type(bpy.types.Operator):
    """Set custom property"""
    bl_idname = "node.game_object_item_type"
    bl_label = "Set custom property"
    bl_description = "Sets a custom property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_item_types)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
    
class NODE_OT_game_object_scripts(bpy.types.Operator):
    """Set custom property"""
    bl_idname = "node.game_object_script"
    bl_label = "Set script property"
    bl_description = "Sets a script property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_scripts)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
    
class NODE_OT_game_object_room_id(bpy.types.Operator):
    """Set room id"""
    bl_idname = "node.game_object_room_id"
    bl_label = "Set room id property"
    bl_description = "Sets a room id property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_rooms)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
    
class NODE_OT_game_object_boolean_variable(bpy.types.Operator):
    """Set boolean variable"""
    bl_idname = "node.game_object_boolean_variable"
    bl_label = "Set boolean variable property"
    bl_description = "Sets a boolean variable property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_booleans)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
         
class NODE_OT_game_object_integer_variable(bpy.types.Operator):
    """Set integer variable"""
    bl_idname = "node.game_object_integer_variable"
    bl_label = "Set integer variable property"
    bl_description = "Sets a integer variable property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_integers)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}

class NODE_OT_game_objects_clear_script(bpy.types.Operator):
    """Set custom property"""
    bl_idname = "node.game_object_clear_script"
    bl_label = "Clear script property"
    bl_description = "Clears a script property on a game object"
    bl_options = {'REGISTER', 'UNDO'}

    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        del context.object[self.name]

        return {'FINISHED'}

class NODE_OT_game_object_entry_points(bpy.types.Operator):
    """Set custom property"""
    bl_idname = "node.game_object_entry_points"
    bl_label = "Set entry point property"
    bl_description = "Sets an entry point property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_entry_points)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
    
class NODE_OT_game_object_positions(bpy.types.Operator):
    """Set custom property"""
    bl_idname = "node.game_object_positions"
    bl_label = "Set position property"
    bl_description = "Sets a position property on a game object"
    bl_property = "selected_item"
    bl_options = {'REGISTER', 'UNDO'}

    selected_item: bpy.props.EnumProperty(items=_get_positions)
    name: bpy.props.StringProperty()

    def execute(self, context):
        if not context.object:
            return
        
        context.object[self.name] = self.selected_item

        return {'FINISHED'}
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}
    
_enum_mapping = {
    'collectable_sub_type': NODE_OT_game_object_item_type.bl_idname,
    'enum inventory_item_type': NODE_OT_game_object_item_type.bl_idname,
    'script_location': NODE_OT_game_object_scripts.bl_idname,
    'room_id': NODE_OT_game_object_room_id.bl_idname,
    'boolean_variable': NODE_OT_game_object_boolean_variable.bl_idname,
    'integer_variable': NODE_OT_game_object_integer_variable.bl_idname,
    'struct Vector3': NODE_OT_game_object_positions.bl_idname,
}

global_attributes = [
    struct_parse.StructureAttribute("condition", "string"),
    struct_parse.StructureAttribute("on_despawn", "boolean_variable"),
]

def _get_obj_def(current_object):
    if not current_object:
        return None

    if current_object.type != 'MESH' and current_object.type != 'CAMERA':
        return None

    if not current_object.data:
        return None
    
    if current_object.data.library:
        return object_definitions.get_object_for_library_path(current_object.data.library.filepath)
    
    if 'type' in current_object:
        return object_definitions.get_object_for_type(current_object['type'])

    return None
        
AUTO_PROPERTIES = {'position', 'rotation', 'scale', 'fov'}

def _init_default_properties(target):
    obj_def = _get_obj_def(target)

    structure = object_definitions.get_struct_info(obj_def['type'])

    if not structure:
        return

    for attr in structure.children + global_attributes:
        if attr.name in AUTO_PROPERTIES:
            continue

        if attr.name in target.data:
            target[attr.name] = target.data[attr.name]
            continue

        if attr.name in target:
            continue
        
        if attr.data_type == 'bool':
            target[attr.name] = False
        elif attr.data_type == 'float':
            target[attr.name] = 0.0
        elif attr.data_type == 'string':
            target[attr.name] = ''
        elif isinstance(attr.data_type, str) and attr.data_type.startswith('enum '):
            default_value = ''
            if attr.data_type in object_definitions.enums:
                default_value = object_definitions.enums[attr.data_type].int_to_str(0)

            target[attr.name] = default_value
        elif attr.data_type == 'collectable_sub_type':
            target[attr.name] = 'ITEM_TYPE_NONE'
        elif attr.data_type == 'script_location':
            target[attr.name] = ''
        elif attr.data_type == 'room_id':
            target[attr.name] = 'room_default'
        elif attr.data_type == 'boolean_variable' or attr.data_type == 'integer_variable':
            target[attr.name] = 'disconnected'
        elif attr.data_type == 'struct Vector3':
            target[attr.name] = ''
        else:
            print('could not generate default for ' + attr.data_type)
            continue


class NODE_OT_game_object_init(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_init"
    bl_label = "Initialize game object properties"
    bl_description = "Sets up a game object's properties"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        if not context.object:
            return
        
        _init_default_properties(context.object)

        return {'FINISHED'}
    
class LoadingZonePanel(bpy.types.Panel):
    bl_idname='GO_PT_loading_zone'
    bl_label='Loading point'
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}
    
    @classmethod
    def poll(cls, context):
        return context.object and 'loading_zone' in context.object
    
    def draw(self, context):
        target = context.object

        self.layout.label(text='Loading zone')
        operator = self.layout.operator(NODE_OT_game_object_entry_points.bl_idname, text=target['loading_zone'])
        operator.name = 'loading_zone'


class EntryPointPanel(bpy.types.Panel):
    bl_idname='GO_PT_entry_point'
    bl_label='Entry point'
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}
    
    @classmethod
    def poll(cls, context):
        return context.object and entry_point.is_entry_point(context.object)
    
    def draw(self, context):
        target = context.object

        if target.name.startswith(entry_point.ENTRY_PREFIX):
            row = self.layout.row()
            row.label(text='Entry point')
            row.label(text=target.name[len(entry_point.ENTRY_PREFIX):])
        elif 'entry_point' in object:
            row = self.layout.row()
            row.label(text='Entry point')
            row.prop(target, '["entry_point"]', text='')

        if 'on_enter' in target:
            row = self.layout.row()
            row.label(text='On enter')
            operator = row.operator(NODE_OT_game_object_scripts.bl_idname, text=target['on_enter'])
            operator.name = 'on_enter'
            operator = row.operator(NODE_OT_game_objects_clear_script.bl_idname, text='', icon='X')
            operator.name = 'on_enter'
        else:
            operator = self.layout.operator(NODE_OT_game_object_scripts.bl_idname, text='Add on enter script')
            operator.name = 'on_enter'
            

class StaticParticlesPanel(bpy.types.Panel):
    bl_idname='GO_PT_static_particles'
    bl_label='Static particles'
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}
    
    @classmethod
    def poll(cls, context):
        return context.object and context.object.type == 'MESH' and 'type' in context.object.data and context.object.data['type'] == 'static_particles'
    
    def draw(self, context):
        target = context.object
        box = self.layout.box()
        split = box.split(factor=0.5)
        split.label(text="Instance mesh")
        split.prop(target, "particle_instance_obj", text="")

    
class GameObjectPanel(bpy.types.Panel):
    bl_idname='GO_PT_game_object'
    bl_label='Game object'
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "object"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        obj_def = _get_obj_def(context.object)
        return obj_def != None
    
    def draw(self, context):
        obj_def = _get_obj_def(context.object)
        structure = object_definitions.get_struct_info(obj_def['type'])

        if not structure:
            return
        
        layout = self.layout
        col = layout.column()
        col.box().label(text=obj_def['name'])
        target = context.object

        is_mising_props = False

        for attr in structure.children + global_attributes:
            if attr.name in AUTO_PROPERTIES:
                continue

            if attr.name in target.data:
                row = col.row()
                row.label(text=attr.name)
                row.label(text=target.data[attr.name])
                continue

            if not attr.name in target:
                row = col.row()
                row.label(text=attr.name)
                row.label(text='missing')
                is_mising_props = True
                continue

            attr_name = f'["{attr.name}"]'

            if attr.data_type in _enum_mapping:
                row = col.row()
                row.label(text=attr.name)
                operator = row.operator(_enum_mapping[attr.data_type], text=target[attr.name])
                operator.name = attr.name
            else:
                row = col.row()
                row.label(text=attr.name)
                row.prop(target, attr_name, text='')

        if is_mising_props:
            layout.operator('node.game_object_init', text='Add missing properties')

class NODE_OT_game_object_add(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_add"
    bl_label = "Add a game object"
    bl_description = "Add a game object"
    bl_property = "name"
    bl_options = {'REGISTER', 'UNDO'}

    name: bpy.props.EnumProperty(
        name="Game object",
        description="Pick a game object from the list",
        items=_get_objects
    )
    
    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}

    def search_for_mesh(self, absolute_path, relative_path, mesh_name):
        for mesh in bpy.data.meshes:
            if mesh.name == mesh_name and mesh.library and (mesh.library.filepath == absolute_path or mesh.library.filepath == relative_path):
                return mesh
        
        return None
    
    def get_or_link_mesh(self, absolute_path, mesh_name):
        relative_path = "//" + os.path.relpath(absolute_path, os.path.dirname(bpy.data.filepath))

        result = self.search_for_mesh(absolute_path, relative_path, mesh_name)

        if result:
            return result

        with bpy.data.libraries.load(relative_path, link=True, relative=True) as (data_from, data_to):
            data_to.meshes = [mesh_name]

        return self.search_for_mesh(absolute_path, relative_path, mesh_name)

    def execute(self, context):
        object_def = None

        for game_object_def in object_definitions.get_objects_list():
            if game_object_def["name"] == self.name:
                object_def = game_object_def
                break;

        if not object_def:
            return {'ERROR'}
        
        filepath = os.path.join("assets", object_def["mesh"])
        name = game_object_def["name"]

        parts = filepath.split('#', 1)
        absolute_path = os.path.join(object_definitions.get_repo_path(), parts[0])

        mesh = self.get_or_link_mesh(absolute_path, parts[1])

        if not mesh:
            return {'ERROR'}

        object = bpy.data.objects.new(name, mesh)
        _init_default_properties(object)

        object.location = context.scene.cursor.location
        bpy.context.collection.objects.link(object)
        object.select_set(True)
        bpy.context.view_layer.objects.active = object

        return {'FINISHED'}
    
class NODE_OT_game_object_add_entry_point(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_add_entry_point"
    bl_label = "Add an entry point"
    bl_description = "Add an entry point"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        object = bpy.data.objects.new('entry#new entry point', None)

        object.location = context.scene.cursor.location
        bpy.context.collection.objects.link(object)
        object.select_set(True)
        bpy.context.view_layer.objects.active = object

        return {'FINISHED'}
    
class NODE_OT_game_object_add_loading_zone(bpy.types.Operator):
    """Add a game object"""
    bl_idname = "node.game_object_add_loading_zone"
    bl_label = "Add a loading zone"
    bl_description = "Add a loading zone"
    bl_options = {'REGISTER', 'UNDO'}

    def execute(self, context):
        bm = bmesh.new()
        bmesh.ops.create_cube(bm, size=1)

        mesh = bpy.data.meshes.new('loading zone')
        bm.to_mesh(mesh)
        mesh.update()
        bm.free()

        object = bpy.data.objects.new('loading zone', mesh)

        object.location = context.scene.cursor.location + mathutils.Vector((0, 0, 1))
        object['loading_zone'] = ''
        bpy.context.collection.objects.link(object)
        object.select_set(True)
        bpy.context.view_layer.objects.active = object

        return {'FINISHED'}

class CreateGameObjectPanel(bpy.types.Panel):
    bl_idname = "GO_PT_create_game_object"
    bl_label = "Create game object"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "Game Object"

    @classmethod
    def poll(cls, context):
        return object_definitions.get_repo_path() != None
    
    def draw(self, context):
        object_definitions.load()
        col = self.layout.column()
        col.operator(NODE_OT_game_object_add.bl_idname, text="Create game object")
        col.operator(NODE_OT_game_object_add_entry_point.bl_idname, text="Add entry point")
        col.operator(NODE_OT_game_object_add_loading_zone.bl_idname, text="Add loading zone")

_classes = [
    NODE_OT_game_object_item_type,
    NODE_OT_game_object_scripts,
    NODE_OT_game_object_room_id,
    NODE_OT_game_object_boolean_variable,
    NODE_OT_game_object_integer_variable,
    NODE_OT_game_objects_clear_script,
    NODE_OT_game_object_entry_points,
    NODE_OT_game_object_add,
    NODE_OT_game_object_add_entry_point,
    NODE_OT_game_object_add_loading_zone,
    NODE_OT_game_object_init,
    NODE_OT_game_object_positions,
    LoadingZonePanel,
    GameObjectPanel,
    EntryPointPanel,
    StaticParticlesPanel,
    CreateGameObjectPanel,
]

def register():
    bpy.types.Object.particle_instance_obj = bpy.props.PointerProperty(
        name="Instnace mesh",
        type=bpy.types.Object,
        description="Select a mesh"
    )
    for cls in _classes:
        bpy.utils.register_class(cls)

def unregister():
    del bpy.types.Object.particle_instance_obj
    for cls in _classes:
        bpy.utils.unregister_class(cls)
