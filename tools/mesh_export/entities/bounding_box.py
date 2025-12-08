import mathutils

def union(a: tuple[mathutils.Vector, mathutils.Vector], b: tuple[mathutils.Vector, mathutils.Vector]) -> tuple[mathutils.Vector, mathutils.Vector]:
    return mathutils.Vector((
        min(a[0].x, b[0].x),
        min(a[0].y, b[0].y),
        min(a[0].z, b[0].z)
    )), mathutils.Vector((
        max(a[1].x, b[1].x),
        max(a[1].y, b[1].y),
        max(a[1].z, b[1].z)
    ))