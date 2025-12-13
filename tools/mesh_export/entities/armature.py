import bpy
import sys
import os
import mathutils
import struct
import math

sys.path.append(os.path.dirname(__file__))

from . import export_settings
from . import material_extract

class BoneLinkage():
    def __init__(self, name: str, bone_index: int, transform: mathutils.Matrix):
        self.name: str = name
        self.bone_index: int = bone_index
        self.transform: mathutils.Matrix = transform

class BoneAttributes():
    def __init__(self):
        # I may make this default to false
        # if I can figure out a good way to 
        # reset the default position at the
        # start of an animation
        self.has_pos = True
        self.has_rot = True
        self.has_scale = False

    def __str__(self) -> str:
        return f"{'pos ' if self.has_pos else ''}{'rot ' if self.has_rot else ''}{'scale' if self.has_scale else ''}"
    
    def to_bytes(self) -> bytes:
        result = 0

        if self.has_pos:
            result |= 128
        if self.has_rot:
            result |= 64
        if self.has_scale:
            result |= 32

        return result.to_bytes(1, 'big')
    
    def determine_attribute_size(self) -> int:
        result = 0

        if self.has_pos:
            result += 6
        if self.has_rot:
            result += 6
        if self.has_scale:
            result += 6

        return result

class ArmatureAttributes():
    def __init__(self, bone_count):
        self.bone_attributes: list[BoneAttributes] = []

        for i in range(bone_count):
            self.bone_attributes.append(BoneAttributes())

        self.has_events: bool = False
        self.has_image_frames: bool = False
        self.has_prim_color: bool = False
        self.has_env_color: bool = False

    def determine_attribute_list_size(self) -> int:
        result = 0

        for attr in self.bone_attributes:
            result += attr.determine_attribute_size()

        if self.has_events:
            result += 2

        if self.has_image_frames:
            result += 2

        if self.has_prim_color:
            result += 4

        if self.has_env_color:
            result += 4

        return result
    
    def determine_used_flags(self) -> int:
        result: int = 0

        if self.has_events:
            result |= 1 << 15
        
        if self.has_image_frames:
            result |= 1 << 14

        if self.has_prim_color:
            result |= 1 << 13
            
        if self.has_env_color:
            result |= 1 << 12

        return result

    
class PackedArmatureData():
    def __init__(self, loc: mathutils.Vector, rot: mathutils.Quaternion, scale: mathutils.Vector):
        packed_rotation = pack_quaternion(rot)
        self._data: list[int] = [
            _pack_position(loc.x),
            _pack_position(loc.y),
            _pack_position(loc.z),
            packed_rotation[0],
            packed_rotation[1],
            packed_rotation[2],
            _pack_scale(scale.x),
            _pack_scale(scale.y),
            _pack_scale(scale.z),
        ]

    def write_to_file(self, file, attributes: BoneAttributes | None = None):
        if not attributes or attributes.has_pos:
            file.write(struct.pack(">hhh",
                self._data[0], self._data[1], self._data[2]                
            ))

        if not attributes or attributes.has_rot:
            file.write(struct.pack(">hhh",
                self._data[3], self._data[4], self._data[5]                
            ))

        if not attributes or attributes.has_scale:
            file.write(struct.pack(">hhh",
                self._data[6], self._data[7], self._data[8]                
            ))


    def determine_needed_channels(self, default_pose: "PackedArmatureData", output: BoneAttributes):
        if default_pose._data[0] != self._data[0] or \
            default_pose._data[1] != self._data[1] or \
            default_pose._data[2] != self._data[2]:
            output.has_pos = True

        if default_pose._data[3] != self._data[3] or \
            default_pose._data[4] != self._data[4] or \
            default_pose._data[5] != self._data[5]:
            output.has_rot = True
        
        if default_pose._data[6] != self._data[6] or \
            default_pose._data[7] != self._data[7] or \
            default_pose._data[8] != self._data[8]:
            output.has_scale = True

    def __str__(self) -> str:
        return f"[{', '.join([str(entry) for entry in self._data])}]"

class PackedEventData():
    def __init__(self):
        self.events: list[str] = []
        self.event_mask: int = 0

    def has_events(self) -> bool:
        return self.event_mask != 0

    def add_event(self, index: int):
        self.event_mask |= 1 << index

    def pack(self):
        return self.event_mask
    
class PackedImageFrameData():
    def __init__(self):
        self.image_frame_0 = None
        self.image_frame_1 = None

class PackedColorData():
    def __init__(self, color: list[float]):
        self.r = int((color[0] ** 0.454545) * 255)
        self.g = int((color[1] ** 0.454545) * 255)
        self.b = int((color[2] ** 0.454545) * 255)
        self.a = int(color[3] * 255)

    def write_to_file(self, file):
        file.write(struct.pack('>BBBB', self.r, self.g, self.b, self.a))

class PackedFrame():
    def __init__(self, pose: list[PackedArmatureData], events: PackedEventData, image_data: PackedImageFrameData, prim_color: PackedColorData | None, env_color: PackedColorData | None):
        self.pose: list[PackedArmatureData] = pose
        self.events: PackedEventData = events
        self.image_data: PackedImageFrameData = image_data
        self.prim_color: PackedColorData | None = prim_color
        self.env_color: PackedColorData | None = env_color

class PackedAnimation():
    def __init__(self):
        self._frames: list[PackedFrame] = []

    def add_frame(self, frame: list[PackedArmatureData], events: PackedEventData, image_data: PackedImageFrameData, prim_color: PackedColorData | None, env_color: PackedColorData | None):
        self._frames.append(PackedFrame(frame, events, image_data, prim_color, env_color))

    def frame_count(self):
        return len(self._frames)

    def determine_needed_channels(self, default_pose: list[PackedArmatureData]) -> ArmatureAttributes:
        result: ArmatureAttributes = ArmatureAttributes(len(default_pose))

        for bone_idx, defualt_bone_pose in enumerate(default_pose):
            bone_attr = result.bone_attributes[bone_idx]

            for frame in self._frames:
                frame.pose[bone_idx].determine_needed_channels(defualt_bone_pose, bone_attr)

        for frame in self._frames:
            if frame.events.has_events():
                result.has_events = True
            if frame.image_data.image_frame_0 != None or frame.image_data.image_frame_1 != None:
                result.has_image_frames = True
            if frame.prim_color:
                result.has_prim_color = True
            if frame.env_color:
                result.has_env_color = True

        return result
    
    def write_to_file(self, file, attributes: ArmatureAttributes):
        for frame in self._frames:
            for bone_idx, bone_pose in enumerate(frame.pose):
                bone_pose.write_to_file(file, attributes.bone_attributes[bone_idx])

            if attributes.has_events:
                file.write(frame.events.pack().to_bytes(2, 'big'))
            if attributes.has_image_frames:
                file.write(struct.pack('>BB', frame.image_data.image_frame_0 or 0, frame.image_data.image_frame_1 or 0))
            if attributes.has_prim_color:
                frame.prim_color.write_to_file(file)
            if attributes.has_env_color:
                frame.env_color.write_to_file(file)

class ArmatureBone:
    def __init__(self, index: int, bone: bpy.types.Bone, pose_bone: bpy.types.PoseBone, armature_transform: mathutils.Matrix):
        self.index: int = index
        self.name: str = bone.name
        self.parent_names: list[str] = []

        curr = bone.parent

        while curr:
            self.parent_names.append(curr.name)
            curr = curr.parent

        self.matrix_world: mathutils.Matrix = armature_transform @ bone.matrix_local
        self.matrix_scene_inv: mathutils.Matrix = self.matrix_world.inverted()
        self.matrix_normal_inv = self.matrix_scene_inv.to_3x3().inverted().transposed()

        self.pose_bone: bpy.types.Bone = pose_bone

    def __str__(self):
        return f"'bone[{self.index}] {self.name}"
    
    def __repr__(self):
        return str(self)
    
event_names = [
    "event_attack",
    "event_step"
]

class ArmatureData:
    def __init__(self, obj: bpy.types.Object, base_transform: mathutils.Matrix):
        self.relative_vertex_transform: mathutils.Matrix = base_transform @ obj.matrix_world
        self.relative_bone_transform: mathutils.Matrix = base_transform @ obj.matrix_world.inverted()
        self.armature: bpy.types.Armature = obj.data
        self.obj: bpy.types.Object = obj
        self.used_bones: set[str] = set()
        self.base_transform: mathutils.Matrix = base_transform

        self._filtered_bones: dict[str, ArmatureBone] = {}
        self._ordered_bones: list[ArmatureBone] = []
        self.image_frame_list_0: list[str] = []
        self.image_frame_list_1: list[str] = []

    def check_connected_mesh_object(self, obj: bpy.types.Object):
        if obj.type != "MESH":
            return

        if obj.parent is None or obj.parent.data != self.armature:
            return

        self.mark_bone_used(obj.parent_bone)

        mesh: bpy.types.Mesh = obj.data

        used_group_indices = set()

        for vertex in mesh.vertices:
            for group in vertex.groups:
                if group.weight > 0:
                    used_group_indices.add(group.group)
                
        for group_index in used_group_indices:
            self.mark_bone_used(obj.vertex_groups[group_index].name)

        for material in mesh.materials:
            parsed_filename = material_extract.load_material_with_name(material)

            if parsed_filename.tex0 and parsed_filename.tex0.frames:
                self.image_frame_list_0 = parsed_filename.tex0.frame_filenames()
            if parsed_filename.tex1 and parsed_filename.tex1.frames:
                self.image_frame_list_1 = parsed_filename.tex1.frame_filenames()


    def mark_bone_used(self, name: str | None):
        if name is None or len(name) == 0:
            return
        
        bone: bpy.types.Bone = self.armature.bones.get(name)

        if bone is None:
            return
        
        # check if already added
        if name is self.used_bones:
            return

        self.used_bones.add(name)

    def build_bone_data(self):
        for idx, bone in enumerate(self.armature.bones):
            if bone.name not in self.used_bones:
                continue

            self._filtered_bones[bone.name] = ArmatureBone(
                len(self._filtered_bones),
                bone,
                self.obj.pose.bones[idx],
                self.relative_vertex_transform
            )

        self._ordered_bones = sorted(list(self._filtered_bones.values()), key = lambda x: x.index)

    def find_bone_data(self, bone_name: str):
        return self._filtered_bones[bone_name]
    
    def find_bone_linkage(self, obj: bpy.types.Object) -> BoneLinkage:

        bone = self.find_bone_data(obj.parent_bone)
        final_transform = bone.pose_bone.matrix.inverted() @ obj.matrix_world @ self.base_transform.inverted()

        return BoneLinkage(
            obj.data.name, 
            bone.index, 
            final_transform
        )
    
    def find_parent_bone(self, bone_name: str):
        curr = self.find_bone_data(bone_name)

        for parent_name in curr.parent_names:
            if parent_name in self.used_bones:
                return self._filtered_bones[parent_name]
            
        return None
    
    def get_filtered_bones(self) -> list[ArmatureBone]:
        return self._ordered_bones
    
    def is_action_compatible(self, action: bpy.types.Action) -> bool:
        if len(action.groups) == 0:
            return False

        for group in action.groups:
            if not group.name in self.armature.bones:
                return False
            
        return True
    
    def generate_pose_data(self, settings: export_settings.ExportSettings) -> list[PackedArmatureData]:
        result: list[PackedArmatureData] = []
        for bone in self._ordered_bones:      
            parent = self.find_parent_bone(bone.name)

            transform = bone.pose_bone.matrix

            if parent:
                transform = parent.pose_bone.matrix.inverted() @ transform 
            else: 
                transform = self.relative_bone_transform @ transform

            loc, rot, scale = transform.decompose()

            result.append(PackedArmatureData(loc * settings.fixed_point_scale, rot, scale))
        return result
    
    def generate_event_data(self) -> PackedEventData:
        result: PackedEventData = PackedEventData()

        for index, name in enumerate(event_names):
            if name in self.obj and self.obj[name]:
                result.add_event(index)

        return result
    
    def generate_image_frame_data(self) -> PackedImageFrameData:
        result: PackedImageFrameData = PackedImageFrameData()

        if 'image_frame_0' in self.obj:
            result.image_frame_0 = self.obj['image_frame_0']
        if 'image_frame_1' in self.obj:
            result.image_frame_0 = self.obj['image_frame_1']

        return result

    def gemerate_prim_frame_data(self) -> PackedColorData:
        if 'prim_color' in self.obj:
            return PackedColorData(self.obj['prim_color'])

        return None
        
    def gemerate_env_frame_data(self) -> PackedColorData:
        if 'env_color' in self.obj:
            return PackedColorData(self.obj['env_color'])

        return None

def get_channels(group: bpy.types.ActionGroup, suffix: str) -> list[bpy.types.FCurve]:
    result = []

    for channel in group.channels:
        if channel.data_path.endswith(suffix):
            result.append(channel)

    result = sorted(result, key=lambda x: x.array_index)

    return result

def _pack_position(input: float) -> int:
    return math.floor(input * 8 + 0.5)

def _pack_scale(input: float) -> int:
    return math.floor(input * 256 + 0.5)

def pack_quaternion(input: mathutils.Quaternion):
    if input.w < 0:
        final_input = -input
    else:
        final_input = input

    return [
        math.floor(32767 * final_input.x + 0.5),
        math.floor(32767 * final_input.y + 0.5),
        math.floor(32767 * final_input.z + 0.5)
    ]

def write_armature(file, arm: ArmatureData | None, settings: export_settings.ExportSettings):
    if arm is None:
        file.write((0).to_bytes(2, 'big'))
        return
    
    bones = arm.get_filtered_bones()
    
    file.write(len(bones).to_bytes(2, 'big'))

    for bone in bones:
        parent = arm.find_parent_bone(bone.name)

        if parent:
            file.write(parent.index.to_bytes(1, 'big'))
        else:
            file.write((255).to_bytes(1, 'big'))

    default_pose = arm.generate_pose_data(settings)

    for bone_pose in default_pose:
        bone_pose.write_to_file(file)

    file.write(struct.pack('>BB', len(arm.image_frame_list_0), len(arm.image_frame_list_1)))

    for image in (arm.image_frame_list_0 + arm.image_frame_list_1):
        encoded = image.encode()
        file.write(len(encoded).to_bytes(1, 'big'))
        file.write(encoded)

    flags = 0

    if 'prim_color' in arm.obj:
        flags |= 1 << 15

    if 'env_color' in arm.obj:
        flags |= 1 << 14

    file.write(struct.pack('>H', flags))
    