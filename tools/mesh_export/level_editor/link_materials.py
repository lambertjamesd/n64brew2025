
import bpy
import os.path
from .definitions import object_definitions

_options = []

def _get_options(self, context):
    global _options

    assets_folder = os.path.join(object_definitions.get_repo_path(), 'assets')
    curr_path = os.path.dirname(bpy.data.filepath)

    _options = list(map(lambda x: (x, os.path.relpath(os.path.join(curr_path, x), assets_folder), ''), object_definitions.get_materials()))
    _options.sort(key=lambda x: x[1])
    return _options

class LinkMaterialOperator(bpy.types.Operator):
    """Link a f3d material"""
    bl_idname = "material.link_material"
    bl_label = "Quick link material"
    bl_property = "material_filename"

    material_filename: bpy.props.EnumProperty(
        name="Material list",
        items = _get_options,
    )

    def execute(self, context):
        with bpy.data.libraries.load("//" + self.material_filename, link=True, relative=True) as (data_from, data_to):
            data_to.materials = data_from.materials
        return {'FINISHED'}

    def invoke(self, context, event):
        context.window_manager.invoke_search_popup(self)
        return {'RUNNING_MODAL'}

def menu_function(self, context):
    self.layout.operator(LinkMaterialOperator.bl_idname, text="Quick link material")

def register():
    bpy.utils.register_class(LinkMaterialOperator)
    bpy.types.TOPBAR_MT_file.append(menu_function)

def unregister():
    bpy.utils.unregister_class(LinkMaterialOperator)
    bpy.types.TOPBAR_MT_file.remove(menu_function)