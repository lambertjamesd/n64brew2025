import bpy
import io
import mathutils
import struct
import sys
import re

sys.path.append("..")

from parse.struct_parse import StructureInfo, EnumInfo
from parse.struct_serialize import layout_strings, SerializeContext, write_obj, get_position, get_value

from cutscene.parser import parse_expression
from cutscene.expresion_generator import generate_script
from cutscene.variable_layout import VariableContext

MULTIROOM_ATTRIBUTE = {
    "door": ["room_a", "room_b"],
    "room_portal": ["room_a", "room_b"],
}

SHOULD_AUTO_GEN_CONDITION = {"biter", "jelly", "burning_thorns"}

class ObjectEntry():
    def __init__(self, obj: bpy.types.Object, name: str, def_type: StructureInfo, room_index: int):
        self.obj: bpy.types.Object = obj
        self.name: str = name
        self.def_type: StructureInfo = def_type
        self.position: mathutils.Vector = get_position(obj)
        self.room_index: int = room_index

        self.condition = obj['condition'] if 'condition' in obj else None
        self.on_despawn = obj['on_despawn'] if 'on_despawn' in obj else None

    def get_multiroom_ids(self, context: SerializeContext):
        if not (self.name in MULTIROOM_ATTRIBUTE):
            return []
        
        attrs = MULTIROOM_ATTRIBUTE[self.name]

        result = []

        for attr in attrs:
            room = get_value(self.obj, attr, 0)

            if isinstance(room, str):
                room = context.search_enums(room)

            result.append(room)

        return result

    def write_condition(self, variable_context: VariableContext, file):
        condition_text = self.condition or 'true'
        condition = parse_expression(condition_text, self.obj.name + ".condition")
        script = generate_script(condition, variable_context, 'int')
        script.serialize(file)

    def write_definition(self, context: SerializeContext, file):
        write_obj(file, self.obj, self.def_type, context)

    def should_auto_gen_condition(self):
        if  not (self.name in SHOULD_AUTO_GEN_CONDITION):
            return False
        
        if self.condition == None or len(self.condition) == 0:
            return True
        
        if self.on_despawn == None or self.on_despawn == 'disconnected':
            return False
        
        return False
    
    def get_on_despawn(self):
        if self.on_despawn == None or self.on_despawn == 'disconnected':
            return None
        
        if self.on_despawn.startswith('global '):
            return self.on_despawn[len('global '): -len(': bool')]
        
        if self.on_despawn.startswith('scene '):
            return self.on_despawn[len('scene '): -len(': bool')]
        
        return None

    
    def generate_spawn_condition(self, variable_name):
        if self.condition == None or len(self.condition) == 0:
            self.condition = f"not {variable_name}"
        if self.on_despawn == None or self.on_despawn == 'disconnected':
            self.on_despawn = f"scene {variable_name}: bool"


def to_cell_pos(value: float, min_pos: float, max_pos: float) -> int:
    norm_value = (value - min_pos) / (max_pos - min_pos)
    int_value = int(255 * norm_value)
    return max(0, min(255, int_value))

def write_object_groups(
        cell_min: mathutils.Vector,
        cell_max: mathutils.Vector,
        objects: list[ObjectEntry], 
        enums: dict[str, EnumInfo], 
        variable_context: VariableContext, 
        first_spawn_id: int, 
        file):
    num_objects = len(objects)
    file.write(num_objects.to_bytes(2, 'big'))

    if num_objects == 0:
        return

    context = SerializeContext(enums)

    definitions = io.BytesIO()
    conditions = io.BytesIO()
    strings = io.BytesIO()

    for object in objects:
        layout_strings(object.obj, object.def_type, context, None)

    for object in objects:
        object.write_condition(variable_context, conditions)
        object.write_definition(context, definitions)

    context.write_strings(strings)

    definition_bytes = definitions.getvalue()
    condition_bytes = conditions.getvalue()
    string_bytes = strings.getvalue()
    file.write(len(definition_bytes).to_bytes(4, 'big'))
    file.write(len(condition_bytes).to_bytes(2, 'big'))
    file.write(len(string_bytes).to_bytes(2, 'big'))

    file.write(definition_bytes)
    file.write(condition_bytes)
    file.write(string_bytes)

    file.write(first_spawn_id.to_bytes(4, 'big'))

    for idx, object in enumerate(objects):
        file.write(struct.pack(
            '>BBH', 
            to_cell_pos(object.position.x, cell_min.x, cell_max.x),
            to_cell_pos(object.position.z, cell_min.z, cell_max.z),
            idx
        ))

    for object in objects:
        def_type = enums['enum entity_type_id'].str_to_int('ENTITY_TYPE_' + object.name)
        file.write(def_type.to_bytes(2, 'big'))
        

    

