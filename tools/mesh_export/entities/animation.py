import bpy

from . import armature
from . import export_settings

def export_animations(filename: str, arm: armature.ArmatureData | None, settings: export_settings.ExportSettings):
    if not arm:
        return
    
    animations = list(filter(lambda action: arm.is_action_compatible(action), bpy.data.actions))

    bones = arm.get_filtered_bones()
    attributes_for_anim: list[armature.ArmatureAttributes] = []
    packed_animations: list[armature.PackedAnimation] = []

    default_pose = arm.generate_pose_data(settings)

    start_action = arm.obj.animation_data.action if arm.obj.animation_data else None
    start_frame = bpy.context.scene.frame_current

    for anim in animations:
        arm.obj.animation_data.action = anim

        frame_start = int(anim.frame_range[0])
        frame_end = int(anim.frame_range[1])

        packed_animation = armature.PackedAnimation()

        for frame in range(frame_start, frame_end):
            bpy.context.scene.frame_set(frame)
            packed_animation.add_frame(arm.generate_pose_data(settings), arm.generate_event_data(), arm.generate_image_frame_data(), arm.gemerate_prim_frame_data(), arm.gemerate_env_frame_data())

        attributes = packed_animation.determine_needed_channels(default_pose)
        attributes_for_anim.append(attributes)
        packed_animations.append(packed_animation)

    if arm.obj.animation_data:
        arm.obj.animation_data.action = start_action
    bpy.context.scene.frame_set(start_frame)

    name_lengths = 0

    for anim in animations:
        name_lengths += len(anim.name.encode()) + 1

    with open(filename, 'wb') as file:
        file.write('ANIM'.encode())
        file.write(len(animations).to_bytes(2, 'big'))
        file.write(name_lengths.to_bytes(2, 'big'))
        file.write(len(bones).to_bytes(2, 'big'))

        # write headers
        for idx, packed_anim in enumerate(packed_animations):
            frame_count = packed_anim.frame_count()
            file.write(frame_count.to_bytes(2, 'big'))
            file.write(bpy.context.scene.render.fps.to_bytes(2, 'big'))
            file.write(attributes_for_anim[idx].determine_attribute_list_size().to_bytes(2, 'big'))
            file.write(attributes_for_anim[idx].determine_used_flags().to_bytes(2, 'big'))

        attr_size = 0
        # write used attributes
        for attributes in attributes_for_anim:
            for attr in attributes.bone_attributes:
                attr_bytes = attr.to_bytes()
                file.write(attr_bytes)
                attr_size += len(attr_bytes)

        for anim in animations:
            name_bytes = anim.name.encode()
            file.write(name_bytes)
            file.write(b'\0')

        for anim_idx, anim in enumerate(packed_animations):
            # align to 2 bytes
            current_size = file.tell()

            if current_size & 1:
                file.write(b'\0')
                
            anim.write_to_file(file, attributes_for_anim[anim_idx])

