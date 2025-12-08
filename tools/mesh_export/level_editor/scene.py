import bpy


enumLightSource = [
    ("none", "None", ""),
    ("from_camera", "From camera", "")
]

class SCENE_PT_spellcraft_settings(bpy.types.Panel):
    bl_label = "Scene inspector"
    bl_idname = "SCENE_PT_spellcraft_settings"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        return context.scene is not None

    def draw(self, context):
        box = self.layout.box()
        scene = context.scene
        split = box.split(factor=0.5)
        split.label(text="Default material")
        split.prop(scene, "default_material", text="")

        split = box.split(factor=0.5)
        split.label(text="Light source")
        split.prop(scene, "light_source", text="")

def register():
    bpy.types.Scene.default_material = bpy.props.PointerProperty(
        name="Default material",
        type=bpy.types.Material,
        description="Select a material"
    )
    bpy.types.Scene.light_source = bpy.props.EnumProperty(
        name="Light source", items=enumLightSource, default="none"
    )
    bpy.utils.register_class(SCENE_PT_spellcraft_settings)

def unregister():
    bpy.utils.unregister_class(SCENE_PT_spellcraft_settings)
    del bpy.types.Scene.default_material
    del bpy.types.Scene.light_source
