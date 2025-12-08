import bpy

import math
import mathutils
import struct
import io
from . import armature

# FOV = 2 * arctan(sensor_size / (2 * focal_length))
SENSOR_SIZE = 36

base_transform = mathutils.Matrix.Rotation(-math.pi * 0.5, 4, 'X')
base_rotation = base_transform.to_quaternion()

class camera_animation_frame:
    def __init__(self):
        self.location = mathutils.Vector()
        self.rotation = mathutils.Quaternion()
        self.has_quat = False
        self.euler_angles = mathutils.Euler()
        self.has_euler = False
        self.fov = 70

    def __str__(self):
        return f"loc = {self.location}, rot = {self.get_rotation()} fov = {self.fov}"
    
    def get_rotation(self):
        if self.has_quat:
            return self.rotation
        if self.has_euler:
            return self.euler_angles.to_quaternion()
        return mathutils.Quaternion()

    def set_value(self, name: str, index: int, value: float):
        if name == 'location':
            self.location[index] = value
        elif name == 'rotation_euler':
            self.euler_angles[index] = value
            self.has_euler = True
        elif name == 'scale':
            # intentionally ignored
            pass
        elif name == 'lens':
            self.fov = 2 * math.atan(0.5 * SENSOR_SIZE / value)
        else:
            raise Exception(f'unknown animation frame value {name}')
        
    def pack(self):
        position = base_transform @ self.location
        rotation = base_rotation @ self.get_rotation()

        if rotation.w < 0:
            rotation = -rotation
        else:
            rotation = rotation
            
        return struct.pack(
            '>fffffff',
            position.x, position.y, position.z,
            rotation.x, rotation.y, rotation.z,
            self.fov
        )

class camera_animation:
    def __init__(self, name: str):
        self.name: str = name
        self.movement_action: bpy.types.Action | None = None
        self.lens_action: bpy.types.Action | None = None

    def start_frame(self):
        return math.floor(min(self.movement_action.frame_range[0], self.lens_action.frame_range[0]))

    def end_frame(self):
        return math.ceil(max(self.movement_action.frame_range[1], self.lens_action.frame_range[1]))
    
    def evaluate(self, frame) -> camera_animation_frame:
        result = camera_animation_frame()
        
        for fcurve in self.movement_action.fcurves:
            result.set_value(fcurve.data_path, fcurve.array_index, fcurve.evaluate(frame))

        for fcurve in self.lens_action.fcurves:
            result.set_value(fcurve.data_path, fcurve.array_index, fcurve.evaluate(frame))

        return result
    
    def pack_animation_data(self) -> bytes:
        result = io.BytesIO()

        for frame in range(self.start_frame(), self.end_frame()):
            result.write(self.evaluate(frame).pack())

        return result.getvalue()
        

def get_camera_animation(actions: dict[str, camera_animation], name: str) -> camera_animation:
    if not name in actions:
        result = camera_animation(name)
        actions[name] = result
        return result
    
    return actions[name]

def export_camera_animations(filename: str, scene_file):
    actions: dict[str, camera_animation] = {}

    for action in bpy.data.actions:
        if action.name.endswith('_cam'):
            get_camera_animation(actions, action.name.removesuffix('_cam')).movement_action = action
        elif action.name.endswith('_lens'):
            get_camera_animation(actions, action.name.removesuffix('_lens')).lens_action = action

    items = actions.items()

    animation_content = []

    for name, anim in items:
        if not anim.movement_action:
            raise Exception(f"animation {name} only has lens data add an action to the camera object that ends in _cam")
        if not anim.lens_action:
            raise Exception(f"animation {name} only has movment data add an action to the camera data that ends in _lens")


        animation_content.append(anim.pack_animation_data())

    scene_file.write(len(items).to_bytes(2, 'big'))

    offset = 0
    index = 0

    for name, anim in items:
        name_bytes = name.encode()
        scene_file.write(len(name_bytes).to_bytes(1, 'big'))
        scene_file.write(name_bytes)

        frame_count = anim.end_frame() - anim.start_frame()
        scene_file.write(frame_count.to_bytes(2, 'big'))
        scene_file.write(offset.to_bytes(4, 'big'))
        offset += len(animation_content[index])
        index += 1

    with open(filename, 'wb') as file:
        for content in animation_content:
            file.write(content)



    