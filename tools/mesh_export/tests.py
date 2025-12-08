import sys
import os.path

sys.path.append(os.path.dirname(__file__))

import mathutils
import io
import entities.mesh_collider

collider = entities.mesh_collider.MeshCollider()

collider.vertices.append(mathutils.Vector((-10, -10, -10)))
collider.vertices.append(mathutils.Vector((10, -10, 10)))
collider.vertices.append(mathutils.Vector((0, 10, 0)))

collider.triangles.append(entities.mesh_collider.MeshColliderTriangle([0, 1, 2]))

file_tmp = io.BytesIO()
collider.write_out(file_tmp)