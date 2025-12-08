
ENTRY_PREFIX='entry#'

def is_entry_point(object):
    return 'entry_point' in object or object.name.startswith(ENTRY_PREFIX)

def get_entry_point(object):
    if object.name.startswith(ENTRY_PREFIX):
        return object.name[len(ENTRY_PREFIX):]
    
    if 'entry_point' in object:
        return object['entry_point']
    
    return None