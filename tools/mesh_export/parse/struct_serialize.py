import bpy
import mathutils
import math
import struct

from . import struct_parse

class SerializeContext():
    def __init__(self, enums):
        self.enums = enums
        self._strings: dict[str, int] = {}
        self._string_data: list[bytes] = []
        self._current_offset = 0
        self._did_write = False

    def get_string_offset(self, value: str):
        if value in self._strings:
            return self._strings[value]
        
        if self._did_write:
            raise Exception(f'tried to layout string {value} after bytes were written')
        
        result = self._current_offset

        self._strings[value] = result
        string_data = value.encode() + bytes(1)
        self._string_data.append(string_data)
        self._current_offset += len(string_data)

        return result
    
    def search_enums(self, value: str):
        for single_enum in self.enums.values():
            if single_enum.is_defined(value):
                return single_enum.str_to_int(value)

        raise Exception(f'{value} is not found in any enum value')

    def get_bytes(self) -> bytes:
        self._did_write = True

        return b''.join(self._string_data)
    
    def write_strings(self, file):
        all_bytes = self.get_bytes()
        file.write(struct.pack('>H', len(all_bytes)))
        file.write(all_bytes)


fixed_sizes = {
    'float': 4,
    'int': 4,
    'uint64_t': 8,
    'uint32_t': 4,
    'uint16_t': 2,
    'uint8_t': 1,
    'int64_t': 8,
    'int32_t': 4,
    'int16_t': 2,
    'int8_t': 1,
    'struct Vector3': 12,
    'struct Vector2': 8,
    'struct Quaternion': 16,
    'collectable_sub_type': 4,
    'room_id': 2,
    'bool': 1,
    'boolean_variable': 2,
    'integer_variable': 2,
}

fixed_alignments = {
    'float': 4,
    'int': 4,
    'uint64_t': 8,
    'uint32_t': 4,
    'uint16_t': 2,
    'uint8_t': 1,
    'int64_t': 8,
    'int32_t': 4,
    'int16_t': 2,
    'int8_t': 1,
    'struct Vector3': 4,
    'struct Vector2': 4,
    'struct Quaternion': 4,
    'collectable_sub_type': 4,
    'room_id': 2,
    'bool': 1,
    'boolean_variable': 2,
    'integer_variable': 2,
}

struct_formats = {
    'float': 'f',
    'int': 'i',
    'uint64_t': 'Q',
    'uint32_t': 'I',
    'uint16_t': 'H',
    'uint8_t': 'B',
    'int64_t': 'q',
    'int32_t': 'i',
    'int16_t': 'h',
    'int8_t': 'b',
    'collectable_sub_type': 'I',
    'room_id': 'H',
    'bool': 'B',
    'boolean_variable': 'H',
    'integer_variable': 'H',
}

struct_format_defaults = {
    'boolean_variable': 0xFFFF,
    'integer_variable': 0xFFFF,
}

_string_aliases = {
    'script_location',
    'scene_entry_point',
}

SENSOR_SIZE = 36

def _is_string_type(definition):
    return isinstance(definition, struct_parse.PointerType) and definition.sub_type == 'char' or definition in _string_aliases

coordinate_convert = mathutils.Matrix.Rotation(math.pi * 0.5, 4, 'X')
coordinate_convert_invert = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')

def get_value(obj: bpy.types.Object, key: str, default_value):
    if key in obj:
        return obj[key]
    
    if key in obj.data:
        return obj.data[key]
    
    return default_value

def _get_transform(obj: bpy.types.Object) -> mathutils.Matrix:
    if obj.type == 'CAMERA':
        return coordinate_convert_invert @ obj.matrix_world
    return coordinate_convert_invert @ obj.matrix_world @ coordinate_convert

def get_position(obj: bpy.types.Object) -> mathutils.Vector:
    loc, rot, scale = _get_transform(obj).decompose()
    return loc

def get_scale(obj: bpy.types.Object) -> mathutils.Vector:
    loc, rot, scale = _get_transform(obj).decompose()
    return scale

def layout_strings(obj: bpy.types.Object, definition, context: SerializeContext, field_name = None):
    if _is_string_type(definition):
        context.get_string_offset(str(get_value(obj, field_name, "")))

    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            layout_strings(obj, child.data_type, context, child.name)

def write_vector3_position(file, obj: bpy.types.Object):
    loc = get_position(obj)
    file.write(struct.pack(">fff", loc.x, loc.y, loc.z))

def write_vector3_scale(file, obj: bpy.types.Object):
    scale = get_scale(obj)
    file.write(struct.pack(">fff", scale.x, scale.y, scale.z))


def write_vector2_rotation(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    rotated_right = rot @ mathutils.Vector([1, 0, 0])

    final_right = mathutils.Vector([rotated_right.x, 0, rotated_right.z]).normalized()

    file.write(struct.pack(">ff", final_right.x, final_right.z))

def write_quaternion_rotation(file, obj: bpy.types.Object):
    loc, rot, scale = _get_transform(obj).decompose()
    file.write(struct.pack(">ffff", rot.x, rot.y, rot.z, rot.w))

def _apply_alignment(current_offset: int, alignment: int) -> int:
    return (current_offset + alignment - 1) & ~(alignment - 1)

def obj_determine_alignment(definition, context: SerializeContext) -> int:
    if _is_string_type(definition):
        return 4
    
    if isinstance(definition, str):
        if definition in fixed_alignments:
            return fixed_alignments[definition]
        if definition in context.enums:
            return 4
        raise Exception(f"{definition} doesn't have a known alignment")
    
    if isinstance(definition, struct_parse.PointerType):
        return 4
    
    if isinstance(definition, struct_parse.StructureInfo):
        if definition.align:
            return definition.align

        struct_alignment = 1
        for child in definition.children:
            struct_alignment = max(struct_alignment, obj_determine_alignment(child.data_type, context))
        definition.align = struct_alignment
        return struct_alignment
    
    raise Exception(f'Unknown type {definition}')

def _write_padding(file, offset: int, definition, context: SerializeContext) -> int:
    new_offset = _apply_alignment(offset, obj_determine_alignment(definition, context))

    if new_offset == offset:
        return offset

    for i in range(new_offset - offset):
        file.write(struct.pack('>B', 0))
    
    return new_offset

def write_obj(file, obj: bpy.types.Object, definition, context: SerializeContext, field_name = None, offset: int = 0) -> int:
    offset = _write_padding(file, offset, definition, context)

    if _is_string_type(definition):
        file.write(struct.pack(">I", context.get_string_offset(str(get_value(obj, field_name, "")))))
        return offset + 4
    
    if isinstance(definition, str):
        if definition == 'struct Vector3':
            if field_name == 'position':
                write_vector3_position(file, obj)
                return offset + 12
            if field_name == 'scale':
                write_vector3_scale(file, obj)
                return offset + 12
            
            value = get_value(obj, field_name, 0)
            if isinstance(value, str):
                if value.startswith('obj '):
                    write_vector3_position(file, bpy.data.objects[value[len('obj '):]])
                else:
                    file.write(struct.pack('>fff', 0, 0, 0))
                return offset + 12
        if definition == 'struct Vector2':
            if field_name == 'rotation':
                write_vector2_rotation(file, obj)
                return offset + 8
        if definition == 'struct Quaternion':
            if field_name == 'rotation':
                write_quaternion_rotation(file, obj)
                return offset + 16
        if definition == 'float':
            if field_name == 'scale':
                value = get_scale(obj).x
            elif field_name == 'fov' and obj.type == 'CAMERA':
                value = obj.data.angle_y * 180 / math.pi
            else:
                value = get_value(obj, field_name, 0)
            file.write(struct.pack(">f", value))
            return offset + 4
        if definition in struct_formats:
            value = get_value(obj, field_name, struct_format_defaults[definition] if definition in struct_format_defaults else 0)

            if value == True:
                value = 1

            if value == False:
                value = 0

            if isinstance(value, str):
                value = context.search_enums(value)

            file.write(struct.pack(">" + struct_formats[definition], value))
            return offset + fixed_sizes[definition]
        if definition in context.enums:
            value = get_value(obj, field_name, None)

            if value == None:
                value = 0
            else:
                value = context.enums[definition].str_to_int(value)

            file.write(struct.pack(">I", value))
            return offset + 4
         
        raise Exception(f"unknown field type '{definition}' {field_name}")
    
    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            offset = write_obj(file, obj, child.data_type, context, child.name, offset)

    if field_name == None:
        # add end of struct padding
        _write_padding(file, offset, definition, context)

TYPE_ID_STR = 0

class TypeLocation():
    def __init__(self, type_id: int, offset: int):
        self.type_id: int = type_id
        self.offset: int = offset

def _obj_gather_types(definition, context: SerializeContext, current_offset: int) -> int:
    current_offset = _apply_alignment(current_offset, obj_determine_alignment(definition, context))

    if _is_string_type(definition):
        return current_offset + 4
    
    if isinstance(definition, str):
        if definition in fixed_sizes:
            return current_offset + fixed_sizes[definition]

        if definition in context.enums:
            return current_offset + 4

        raise Exception(f"{definition} is not a known size")
    
    if isinstance(definition, struct_parse.PointerType):
        return current_offset + 4, 4
    
    if isinstance(definition, struct_parse.StructureInfo):
        for child in definition.children:
            current_offset = _obj_gather_types(child.data_type, context, current_offset = current_offset)

        return current_offset
    
    raise Exception(f'Unknown type {definition}')

def obj_gather_types(definition, context: SerializeContext) -> int:
    result = _obj_gather_types(definition, context, 0)
    return _apply_alignment(result, obj_determine_alignment(definition, context))