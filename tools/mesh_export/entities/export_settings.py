import mathutils
from . import material

class ExportSettings():
    def __init__(self):
        self.fixed_point_scale: int = 64
        self.world_scale: int = 32
        self.default_material: material.Material = material.Material()
        self.default_material_name: str = 'rom:/materials/default.mat'
        self.sort_direction: mathutils.Vector | None = None
        self.fog_scale = 1
        self.light_source = 0

    def copy(self):
        result = ExportSettings()

        result.fixed_point_scale = self.fixed_point_scale
        result.world_scale = self.world_scale
        result.default_material = self.default_material
        result.default_material_name = self.default_material_name
        result.sort_direction = self.sort_direction
        result.fog_scale = self.fog_scale
        result.light_source = self.light_source

        return result