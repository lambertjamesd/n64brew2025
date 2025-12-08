import bpy

def determine_room_for_obj(obj: bpy.types.Object) -> str:
    for collection in bpy.data.collections:
        if obj.name in collection.all_objects:
            if collection.name.startswith("room_"):
                return collection.name

    return "room_default"

class room_collection():
    def __init__(self):
        self.rooms: list[str] = []
        self.room_to_index: dict[str, int] = {}

    def get_room_index(self, name: str) -> int:
        if name in self.room_to_index:
            return self.room_to_index[name]
        
        result = len(self.rooms)
        self.room_to_index[name] = result
        self.rooms.append(name)
        return result

    def get_obj_room_index(self, obj: bpy.types.Object) -> int:
        return self.get_room_index(determine_room_for_obj(obj))