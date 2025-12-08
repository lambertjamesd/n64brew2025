import bpy


enumSurfaceType = [
    ("default", "Default", ""),
    ("holey", "Holey", ""),
    ("water", "Water", ""),
    ("sticky", "Sticky", ""),
]

class MATERIAL_PT_spellcraft_collision(bpy.types.Panel):
    bl_label = "Collision Inspector"
    bl_idname = "MATERIAL_PT_spellcraft_collision"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "material"
    bl_options = {"HIDE_HEADER"}

    @classmethod
    def poll(cls, context):
        return context.material is not None

    def draw(self, context):
        box = self.layout.box()
        material = context.material
        split = box.split(factor=0.5)
        split.label(text="Surface Type")
        split.prop(material, "surface_type", text="")

def register():
    bpy.types.Material.surface_type = bpy.props.EnumProperty(
        name="Surface Type", items=enumSurfaceType, default="default"
    )
    bpy.utils.register_class(MATERIAL_PT_spellcraft_collision)

def unregister():
    bpy.utils.unregister_class(MATERIAL_PT_spellcraft_collision)
    del bpy.types.Material.surface_type
