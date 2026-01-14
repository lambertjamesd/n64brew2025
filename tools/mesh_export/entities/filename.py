import sys
import os

def rom_filename(library_path: str) -> str:
    input_filename = sys.argv[1]
    image_path = os.path.normpath(os.path.join(os.path.dirname(input_filename), library_path[2:]))

    assets_index = image_path.index('assets/')
    return 'rom:/' + image_path[assets_index + len('assets/'):-len('.blend')]
