import json
import os.path
import numbers
import struct
import re
import json

class PngMetadata():
    def __init__(self, filename):
        self.palette: list[int] | None = None
        self.width: int = 0
        self.height: int = 0
        self.fmt: str = 'FMT_NONE'

        self._parse_data(filename)

    def _parse_data(self, filename):
        with open(filename, 'rb') as f:
            data = f.read()
        
        if data[:8] != b'\x89PNG\r\n\x1a\n':
            return
        
        index = 8  # Skip PNG signature
        palette = []
        transparency = []
        
        while index < len(data):
            chunk_length = struct.unpack('>I', data[index:index+4])[0]
            chunk_type = data[index+4:index+8].decode('ascii')
            chunk_data = data[index+8:index+8+chunk_length]
            
            if chunk_type == 'PLTE':
                # Palette contains RGB triplets
                palette = [chunk_data[i:i+3] for i in range(0, len(chunk_data), 3)]
            elif chunk_type == 'tRNS':
                transparency = list(chunk_data)
            elif chunk_type == 'IHDR':
                self.width = struct.unpack('>I', chunk_data[0:4])[0]
                self.height = struct.unpack('>I', chunk_data[4:8])[0]
                self.fmt = self._get_meta_format(filename) or self._get_format_from_png(chunk_data[8], chunk_data[9])
            
            index += 8 + chunk_length + 4  # Move to next chunk (including CRC)
        
        self.palette = self._get_palette_from_png(palette, transparency)

    def _get_palette_from_png(self, palette, transparency):
        if not palette or len(palette) == 0:
            return None
        
        result = []

        for index, entry in enumerate(palette):
            r = entry[0]
            g = entry[1]
            b = entry[2]
            a = transparency[index] if index < len(transparency) else 255

            r5 = (r >> 3) & 0x1F  # 5 bits
            g5 = (g >> 3) & 0x1F  # 5 bits
            b5 = (b >> 3) & 0x1F  # 5 bits
            a1 = (a >> 7) & 0x01  # 1 bit
            
            result.append((r5 << 11) | (g5 << 6) | (b5 << 1) | a1)

        return result

    # Color    Allowed    Interpretation
    # Type    Bit Depths
    # 0       1,2,4,8,16  Each pixel is a grayscale sample.
    # 2       8,16        Each pixel is an R,G,B triple.
    # 3       1,2,4,8     Each pixel is a palette index;
    #                     a PLTE chunk must appear.
    # 4       8,16        Each pixel is a grayscale sample,
    #                     followed by an alpha sample.
    # 6       8,16        Each pixel is an R,G,B triple,
    #                     followed by an alpha sample.

    def _get_format_from_png(self, bit_depth, color_type):
        if color_type == 0:
            return 'FMT_I4' if bit_depth <= 4 else 'FMT_I8'
        if color_type == 2:
            return 'FMT_RGBA16'
        if color_type == 3:
            return 'FMT_CI4' if bit_depth <= 4 else 'FMT_CI8'
        if color_type == 4:
            return 'FMT_IA16'
        if color_type == 6:
            return 'FMT_RGBA16'
        
        return 'FMT_RGBA16'

    def _get_meta_format(self, filename):
        json_filename = filename.replace('.png', '.json')
        if not os.path.isfile(json_filename):
            return None
        with open(json_filename, 'r') as file:
            metadata = json.load(file)
        return 'FMT_' + metadata['format']

class Color():
    def __init__(self, r, g, b, a):
        self.r = r
        self.g = g
        self.b = b
        self.a = a

    def copy(self):
        return Color(self.r, self.g, self.b, self.a)

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Color):
            return False
        
        return self.r == value.r and self.g == value.g and self.b == value.b and self.a == value.a
    
    def __str__(self):
        return f"Color({self.r} {self.g} {self.b} {self.a})"

class CombineModeCycle():
    def __init__(self, a, b, c, d, aa, ab, ac, ad):
        self.a = a
        self.b = b
        self.c = c
        self.d = d

        self.aa = aa
        self.ab = ab
        self.ac = ac
        self.ad = ad

    def copy(self):
        return CombineModeCycle(
            self.a,
            self.b,
            self.c,
            self.d,

            self.aa,
            self.ab,
            self.ac,
            self.ad
        )

    def __str__(self):
        return f"({self.a} {self.b} {self.c} {self.d}) ({self.aa} {self.ab} {self.ac} {self.ad})"
    
    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineModeCycle):
            return False
        
        return self.a == value.a and self.b == value.b and self.c == value.c and self.d == value.d and \
            self.aa == value.aa and self.ab == value.ab and self.ac == value.ac and self.ad == value.ad

    def uses(self, attr: str):
        return self.a == attr or self.b == attr or self.c == attr or self.c == f'{attr}_ALPHA' or self.d == attr or \
            self.aa == attr or self.ab == attr or self.ac == attr or self.ad == attr

class CombineMode():
    def __init__(self, cyc1: CombineModeCycle | None, cyc2: CombineModeCycle | None):
        self.cyc1: CombineModeCycle | None = cyc1
        self.cyc2: CombineModeCycle | None = cyc2

    def copy(self):
        return CombineMode(self.cyc1.copy() if self.cyc1 else None, self.cyc2.copy() if self.cyc2 else None)

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, CombineMode):
            return False

        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2}"
        
        return f"1 cycle {self.cyc1}"
    
    def uses(self, attr: str) -> bool:
        if attr == 'TEX1':
            return bool(self.cyc1 and self.cyc1.uses(attr))
        if attr == 'TEX0':
            return bool(self.cyc1 and self.cyc1.uses(attr) or (self.cyc2 and self.cyc2.uses('TEX1')))

        return bool((self.cyc1 and self.cyc1.uses(attr)) or (self.cyc2 and self.cyc2.uses(attr)))

    
class BlendModeCycle():
    def __init__(self, a1, b1, a2, b2):
        self.a1 = a1
        self.b1 = b1
        self.a2 = a2
        self.b2 = b2

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, BlendModeCycle):
            return False
        
        return self.a1 == value.a1 and self.b1 == value.b1 and self.a2 == value.a2 and self.b2 == value.b2

    def needs_read(self):
        return self.a1 == 'MEMORY' or self.a2 == 'MEMORY' or self.b1 == 'MEMORY_CVG' or self.b2 == 'MEMORY_CVG'
    
    def __str__(self):
        return f"({self.a1} {self.b1} {self.a2} {self.b2})"
    
    def copy(self):
        return BlendModeCycle(self.a1, self.b1, self.a2, self.b2)

class BlendMode():
    def __init__(
            self, 
            cyc1: BlendModeCycle, 
            cyc2: BlendModeCycle | None,
            z_mode: str = 'OPAQUE',
            z_write: bool = True,
            z_compare: bool = True,
            aa: bool = False,
            alpha_compare: str = 'NONE',
            coverage_dest: str = 'CLAMP',
            color_on_coverage: bool = False,
            x_coverage_alpha: bool = False,
            alpha_coverage: bool = False,
            force_blend: bool = False,
            image_read: bool = False
        ):
        self.cyc1: BlendModeCycle = cyc1
        self.cyc2: BlendModeCycle | None = cyc2
        self.z_mode = z_mode
        self.z_write = z_write
        self.z_compare: bool = z_compare
        self.aa: bool = aa,
        self.alpha_compare = alpha_compare
        self.coverage_dest = coverage_dest
        self.color_on_coverage = color_on_coverage
        self.x_coverage_alpha = x_coverage_alpha
        self.alpha_coverage = alpha_coverage
        self.force_blend = force_blend
        self.image_read = image_read

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, BlendMode):
            return False
        
        return self.cyc1 == value.cyc1 and self.cyc2 == value.cyc2 and self.alpha_compare == value.alpha_compare and \
            self.z_mode == value.z_mode and self.z_write == value.z_write and self.z_compare == value.z_compare and \
            self.aa == value.aa and self.coverage_dest == value.coverage_dest and self.color_on_coverage == value.color_on_coverage and \
            self.x_coverage_alpha == value.x_coverage_alpha and self.alpha_coverage == value.alpha_coverage and \
            self.force_blend == value.force_blend and self.image_read == value.image_read

    def __str__(self):
        if self.cyc2:
            return f"2 cycle {self.cyc1} {self.cyc2} {self.z_mode}"
        
        return f"1 cycle {self.cyc1} {self.z_mode}"
    
    def copy(self):
        result = BlendMode(
            self.cyc1 and self.cyc1.copy() or None,
            self.cyc2 and self.cyc2.copy() or None
        )

        result.z_mode = self.z_mode
        result.z_write = self.z_write
        result.z_compare = self.z_compare
        result.aa = self.aa
        result.alpha_compare = self.alpha_compare
        result.coverage_dest = self.coverage_dest
        result.color_on_coverage = self.color_on_coverage
        result.x_coverage_alpha = self.x_coverage_alpha
        result.alpha_coverage = self.alpha_coverage
        result.force_blend = self.force_blend
        result.image_read = self.image_read

        return result
    
    def enable_fog(self):
        result = self.copy()

        result.cyc1 = BlendModeCycle(
            'IN',
            'SHADE_A',
            'FOG',
            'INV_MUX_A'
        )

        return result
    
class Palette():
    def __init__(self):
        self.colors: list[Color] = []
    
class TexAxis():
    def __init__(self):
        self.min = 0 # were translate
        self.max = 0
        self.shift = 0 # scale_log
        self.clamp = False # was repeats
        self.mask = 0 # was repeats
        self.mirror = False
        self.scroll = 0

    def __eq__(self, value: object) -> bool:
        if not isinstance(value, TexAxis):
            return False

        return self.min == value.min and \
            self.max == value.max and \
            self.shift == value.shift and \
            self.clamp == value.clamp and \
            self.mask == value.mask and \
            self.mirror == value.mirror

_fmt_bit_size = {
    'FMT_NONE': 0,
    'FMT_RGBA16': 16,
    'FMT_RGBA32': 32,
    'FMT_YUV16': 16,
    'FMT_CI4': 4,
    'FMT_CI8': 8,
    'FMT_IA4': 4,
    'FMT_IA8': 8,
    'FMT_IA16': 16,
    'FMT_I4': 4,
    'FMT_I8': 8,
}
    
class Tex():
    def __init__(self):
        self.filename: str | None = None
        self.tmem_addr = 0
        self.palette = 0
        self.min_filter = "linear"
        self.mag_filter = "linear"
        self.s = TexAxis()
        self.t = TexAxis()
        self.frames: list[str] | None = None
        self.palette_data = None
        self.fmt = 'FMT_NONE'
        self.width = 128
        self.height = 128

        self._png_metadata: PngMetadata | None = None

    def copy(self):
        result = Tex()
        result.filename = self.filename
        result.tmem_addr = self.tmem_addr
        result.palette = self.palette
        result.min_filter = self.min_filter
        result.mag_filter = self.mag_filter
        result.s = self.s
        result.t = self.t
        result.frames = self.frames
        result.palette_data = self.palette_data
        result.fmt = self.fmt
        result.width = self.width
        result.height = self.height
        result._png_metadata = self._png_metadata
        return result
    
    def set_filename(self, filename):
        self.filename = filename

        if not filename:
            self._png_metadata = None

        self._png_metadata = PngMetadata(filename)
        self.fmt = self._png_metadata.fmt
        self.width = self._png_metadata.width
        self.height = self._png_metadata.height
        self.palette_data = self._png_metadata.palette

    def set_frames(self, frames):
        self.frames = frames

        self._png_metadata = PngMetadata(frames[0])
        self.fmt = self._png_metadata.fmt
        self.width = self._png_metadata.width
        self.height = self._png_metadata.height
        self.palette_data = self._png_metadata.palette

    def get_palette_group(self):
        if not self.filename:
            return None

        result = re.fullmatch("(.*_)palette\\d+\\.png", self.filename)

        if not result:
            return result
        
        return result.group(1)

    def does_share_image_data(self, other):
        self_palette_info = self.get_palette_group()
        other_palette_info = other.get_palette_group()
        return bool(self_palette_info) and self_palette_info == other_palette_info
    
    def has_only_palette(self):
        return bool(self.palette_data) and not self.filename and self.frames

    def rom_filename(self) -> str:
        if not self.filename:
            return ""

        return f"rom:{self.filename[len('assets'):-len('.png')]}.sprite"
        
    def frame_filenames(self) -> list[str]:
        if not self.frames:
            return []

        return list(map(lambda x: f"rom:{x[len('assets'):-len('.png')]}.sprite", self.frames))

    def byte_size(self) -> int:
        return _fmt_bit_size[self.fmt] * self.width * self.height // 8
    

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Tex):
            return False

        return self.filename == value.filename and \
            self.palette_data == value.palette_data and \
            self.palette == value.palette and \
            self.fmt == value.fmt and \
            self.width == value.width and \
            self.height == value.height and \
            self.s == value.s and \
            self.t == value.t and \
            self.frames == value.frames
                

    def __str__(self):
        return f"{self.filename} palette({len(self.palette_data) if self.palette_data else 0}) fmt={self.fmt}"
    
class Fog():
    def __init__(self):
        self.enabled: bool = False
        self.use_global: bool = False
        self.fog_color: Color | None = None
        self.min_distance: float = 0
        self.max_distance: float = 0

    def copy(self):
        result = Fog()
        result.enabled = self.enabled
        result.use_global = self.use_global
        result.fog_color = self.fog_color.copy() if self.fog_color else None
        result.min_distance = self.min_distance
        result.max_distance = self.max_distance
        return result

    def __eq__(self, value: object) -> bool:
        if not value or not isinstance(value, Fog):
            return False
        
        return self.enabled == value.enabled and \
            self.use_global == value.use_global and \
            self.fog_color == value.fog_color and \
            self.min_distance == value.min_distance and \
            self.max_distance == value.max_distance
    
    def __str__(self):
        return f"enabled {str(self.enabled)} use_global {str(self.use_global)} {str(self.fog_color)} min {str(self.min_distance)} max {str(self.max_distance)}"

class Material():
    def __init__(self):
        self.combine_mode: CombineMode | None = None
        self.blend_mode: BlendMode | None = None
        self.env_color: Color | None = None
        self.prim_color: Color | None = None
        self.blend_color: Color | None = None
        self.lighting: bool | None = None
        self.tex0: Tex | None = None
        self.tex1: Tex | None = None
        self.palette: int = 0
        self.culling: bool | None = None
        self.z_buffer: bool | None = None
        self.vtx_effect: str | None = None
        self.fog: Fog | None = None
        self.light_count: int | None = None

    def copy(self):
        result = Material()
        result.combine_mode = self.combine_mode.copy() if self.combine_mode else None
        result.blend_mode = self.blend_mode.copy() if self.blend_mode else None
        result.env_color = self.env_color.copy() if self.env_color else None
        result.prim_color = self.prim_color.copy() if self.prim_color else None
        result.blend_color = self.blend_color.copy() if self.blend_color else None
        result.lighting = self.lighting
        result.tex0 = self.tex0.copy() if self.tex0 else None
        result.tex1 = self.tex1.copy() if self.tex1 else None
        result.palette = self.palette
        result.culling = self.culling
        result.z_buffer = self.z_buffer
        result.vtx_effect = self.vtx_effect
        result.fog = self.fog.copy() if self.fog else None
        result.light_count = self.light_count
        return result

    def combine_mode_uses(self, attr: str) -> bool:
        if not self.combine_mode:
            return False
        
        return self.combine_mode.uses(attr)

    def layout_textures(self):
        if self.tex0 and self.tex1:
            if self.tex0.does_share_image_data(self.tex1):
                self.tex1.filename = None
            else:
                self.tex1.tmem_addr += _align_memory(self.tex0.byte_size())
            if self.tex0.palette_data and len(self.tex0.palette_data):
                self.tex1.palette = 1

    def get_image_size(self) -> tuple[int, int]:
        if self.tex0:
            return (self.tex0.width, self.tex0.height)
        
        return 128, 128
    
    def get_render_layer(self) -> int:
        if not self.blend_mode:
            return 0
        
        if self.blend_mode.z_mode == 'OPAQUE':
            return 0
        if self.blend_mode.z_mode == 'INTER':
            return 0
        if self.blend_mode.z_mode == 'TRANSPARENT':
            return 1
        if self.blend_mode.z_mode == 'DECAL':
            return 2
        
        return 0
    
    def get_palette_data(self) -> tuple[list[int] | None, int]:
        if not self.tex0 and not self.tex1:
            return None, 0
        
        if self.tex0 and not self.tex1:
            return self.tex0.palette_data, self.palette * 16
        
        if self.tex1 and not self.tex0:
            return self.tex1.palette_data, self.palette * 16
        
        result = []

        for tex in [self.tex0, self.tex1]:
            if not tex:
                continue

            palette_data = tex.palette_data

            if not palette_data:
                continue

            index_start = tex.palette * 16

            while len(result) < index_start + len(palette_data):
                result.append(None)

            for idx, value in enumerate(palette_data):
                result[index_start + idx] = value

        index_start = 0

        while index_start < len(result) and result[index_start] == None:
            index_start += 1

        return list(map(lambda x: x or 0, result[index_start:])), index_start

    def is_empty(self):
        return self.combine_mode == None and self.blend_color == None and self.env_color == None and self.prim_color == None and \
            self.blend_color == None and self.lighting == None and self.tex0 == None and self.tex1 == None and self.culling == None and \
            self.z_buffer == None

    def __str__(self):
        return f"""Material:
    combine_mode = {self.combine_mode}
    blend_mode = {self.blend_mode}
    env_color = {self.env_color}
    prim_color = {self.prim_color}
    blend_color = {self.blend_color}
    culling = {self.culling}
    lighting = {self.lighting}
    tex0 = {self.tex0}
    tex1 = {self.tex1}
    fog = {self.fog}
    vtx_effect = {self.vtx_effect}
"""

def _check_is_enum(value, key_path, enum_list):
    if value in enum_list:
        return
    
    raise Exception(f"{key_path} is not a valid value. got '{value}' expected {', '.join(enum_list)}")

def _check_is_int(value, key_path, min, max):
    if not isinstance(value, int) or value < min or value > max:
        raise Exception(f"{key_path} must be an int between {min} and {max}")

def _check_is_number(value, key_path):
    if not isinstance(value, numbers.Real):
        raise Exception(f"{key_path} must be a number")
    
def _optional_number(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_number(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_boolean(value, key_path):
    if not isinstance(value, bool):
        raise Exception(f"{key_path} must be a boolean")
    
def _optional_boolean(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_boolean(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _check_is_string(value, key_path):
    if not isinstance(value, str):
        raise Exception(f"{key_path} must be a string")
    
def _optional_string(json_data, key, key_path, defaultValue):
    if key in json_data:
        _check_is_string(json_data[key], f"{key_path}.{key}")
        return json_data[key]
    
    return defaultValue

def _parse_combine_mode_cycle(json_data, key_path):
    color = json_data['color']

    if not color:
        raise Exception(f"{key_path}.color must be defined")
    
    if not isinstance(color, list) or len(color) != 4:
        raise Exception(f"{key_path}.color must be an array of length 4")
    
    _check_is_enum(
        color[0], 
        f'{key_path}.color[0]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "NOISE",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        color[1], 
        f'{key_path}.color[1]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "CENTER",
            "K4",
            "0"
        ],
    )

    _check_is_enum(
        color[2], 
        f'{key_path}.color[2]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "SCALE",
            "COMBINED_ALPHA",
            "TEX0_ALPHA",
            "TEX1_ALPHA",
            "PRIM_ALPHA",
            "SHADE_ALPHA",
            "ENV_ALPHA",
            "LOD_FRACTION",
            "PRIM_LOD_FRAC",
            "K5",
            "0"
        ],
    )

    _check_is_enum(
        color[3], 
        f'{key_path}.color[3]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )
    
    alpha = json_data['alpha']

    if not alpha:
        alpha = ["0", "0", "0", "1"]

    if not isinstance(alpha, list) or len(alpha) != 4:
        raise Exception(f"{key_path}.alpha must be an array of length 4")

    _check_is_enum(
        alpha[0], 
        f'{key_path}.alpha[0]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        alpha[1], 
        f'{key_path}.alpha[1]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    _check_is_enum(
        alpha[2], 
        f'{key_path}.alpha[2]', 
        [
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "LOD_FRACTION",
            "PRIM_LOD_FRAC",
            "0"
        ],
    )

    _check_is_enum(
        alpha[3], 
        f'{key_path}.alpha[3]', 
        [
            "COMBINED",
            "TEX0",
            "TEX1",
            "PRIM",
            "SHADE",
            "ENV",
            "1",
            "0"
        ],
    )

    return CombineModeCycle(
        color[0],
        color[1],
        color[2],
        color[3],

        alpha[0],
        alpha[1],
        alpha[2],
        alpha[3],
    )

def _parse_combine_mode(result: Material, json_data):
    if not 'combineMode' in json_data:
        return
    
    combine_mode = json_data['combineMode']
    
    if isinstance(combine_mode, list):
        if len(combine_mode) > 2:
            raise Exception('combineMode can only have up to two cycles')

        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode[0], 'combineMode[0]'),
            _parse_combine_mode_cycle(combine_mode[1], 'combineMode[1]') if len(combine_mode) == 2 else None,
        )
    else:
        result.combine_mode = CombineMode(
            _parse_combine_mode_cycle(combine_mode, 'combineMode'),
            None
        )

def _parse_blend_mode_cycle(json_data, key_path):
    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} must be an array of length 4")

    _check_is_enum(
        json_data[0],
        f"{key_path}[0]",
        ["IN", "MEMORY", "BLEND", "FOG"]
    )

    _check_is_enum(
        json_data[1],
        f"{key_path}[1]",
        ["IN_A", "FOG_A", "SHADE_A", "0", "ZERO"]
    )

    _check_is_enum(
        json_data[2],
        f"{key_path}[2]",
        ["IN", "MEMORY", "BLEND", "FOG"]
    )

    _check_is_enum(
        json_data[3],
        f"{key_path}[3]",
        ["INV_MUX_A", "MEMORY_CVG", "ONE", "1", "ZERO", "0"]
    )

    return BlendModeCycle(json_data[0], json_data[1], json_data[2], json_data[3])

def _parse_blend_mode(result: Material, json_data):
    if not 'blendMode' in json_data:
        return
    
    blend_mode = json_data['blendMode']

    if blend_mode == 'OPAQUE':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "0", "IN", "1"), None)
        return

    if blend_mode == 'TRANSPARENT':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "IN_A", "MEMORY", "INV_MUX_A"), None)
        result.blend_mode.z_write = False
        result.blend_mode.z_mode = 'TRANSPARENT'
        return
    
    if blend_mode == 'ALPHA_CLIP':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "0", "IN", "1"), None)
        result.blend_mode.alpha_compare = 'THRESHOLD'
        result.blend_color = Color(0, 0, 0, 128)
        return
    
    if blend_mode == 'ADD':
        result.blend_mode = BlendMode(BlendModeCycle("IN", "IN_A", "MEMORY", "1"), None)
        result.blend_mode.z_write = False
        result.blend_mode.z_mode = 'TRANSPARENT'
        return

    if not 'cyc1' in blend_mode:
        raise Exception(f"blendMode must have at least 1 cycle")
    
    cyc1 = _parse_blend_mode_cycle(blend_mode['cyc1'], 'blendMode.cyc1')
    cyc2 = None

    if 'cyc2' in blend_mode:
        cyc2 = _parse_blend_mode_cycle(blend_mode['cyc2'], 'blendMode.cyc2')

    result.blend_mode = BlendMode(cyc1, cyc2)

    if 'zMode' in blend_mode:
        _check_is_enum(blend_mode['zMode'], 'blendMode.zMode', ['OPAQUE', 'INTER', 'TRANSPARENT', 'DECAL'])
        result.blend_mode.z_mode = blend_mode['zMode']

    if 'zWrite' in blend_mode:
        _check_is_boolean(blend_mode['zWrite'], 'blendMode.zWrite')
        result.blend_mode.z_write = blend_mode['zWrite']

    if 'zCompare' in blend_mode:
        _check_is_boolean(blend_mode['zCompare'], 'blendMode.zCompare')
        result.blend_mode.z_compare = blend_mode['zCompare']

    if 'alphaCompare' in blend_mode:
        _check_is_enum(blend_mode['alphaCompare'], 'blendMode.alphaCompare', ['NONE', 'THRESHOLD', 'NOISE'])


def _parse_color(json_data, key_path):
    if not json_data:
        return None
    
    if not isinstance(json_data, list) or len(json_data) != 4:
        raise Exception(f"{key_path} should be an array of 4 number [0,255]")
    
    _check_is_int(json_data[0], f"{key_path}[0]", 0, 255)
    _check_is_int(json_data[1], f"{key_path}[1]", 0, 255)
    _check_is_int(json_data[2], f"{key_path}[2]", 0, 255)
    _check_is_int(json_data[3], f"{key_path}[3]", 0, 255)

    return Color(json_data[0], json_data[1], json_data[2], json_data[3])

def log_pow_2(value) -> int:
    result = 0
    while (1 << result) < value:
        result += 1
    return result

def _parse_tex_axis(json_data, image_size, into: TexAxis, key_path):
    into.min = 0
    into.max = image_size << 2
    into.shift = _optional_number(json_data, 'shift', key_path, into.shift)
    into.mirror = _optional_boolean(json_data, 'mirror', key_path, False)
    into.scroll = _optional_number(json_data, 'scroll', key_path, into.scroll)

    repeats = _optional_number(json_data, 'repeats', key_path, 2048)
    into.mask = log_pow_2(image_size)
    if repeats:
        into.clamp = False
    else:
        into.clamp = True

def _resolve_tex(filename: str, relative_to: str) -> str:
    combined_path = os.path.join(os.path.dirname(relative_to), filename)
    combined_path = os.path.normpath(combined_path)

    if not os.path.exists(combined_path):
        raise Exception(f"The image file {combined_path} does not exist")
    
    if not combined_path.startswith('assets'):
        raise Exception(f"The image {combined_path} must be in the assets folder")
    
    if not combined_path.endswith('.png'):
        raise Exception(f"The image {combined_path} must be a png file")
    
    return combined_path

def _parse_tex(json_data, key_path, relative_to):
    if not json_data:
        return None
    
    result = Tex()

    if isinstance(json_data, str):
        result.set_filename(_resolve_tex(json_data, relative_to))

        result.s.max = result.width << 2
        result.s.mask = log_pow_2(result.width)
        result.t.max = result.height << 2
        result.t.mask = log_pow_2(result.height)
    else:
        if not 'filename' in json_data and not 'frames' in json_data:
            raise Exception(f"{key_path}.filename or .frames must be defined")
        
        if 'filename' in json_data:
            _check_is_string(json_data['filename'], f"{key_path}.filename")
            result.set_filename(_resolve_tex(json_data['filename'], relative_to))

        if 'tmemAddr' in json_data:
            _check_is_int(json_data['tmemAddr'], f"{key_path}.tmemAddr", 0, 4096)
            result.tmem_addr = json_data['tmemAddr']

        if 'minFilter' in json_data:
            _check_is_enum(json_data['minFilter'], f"{key_path}.minFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['minFilter']

        if 'magFilter' in json_data:
            _check_is_enum(json_data['magFilter'], f"{key_path}.magFilter", ['nearest', 'linear'])
            result.tmem_addr = json_data['magFilter']

        if 's' in json_data:
            _parse_tex_axis(json_data['s'], result.width, result.s, f"{key_path}.s")

        if 't' in json_data:
            _parse_tex_axis(json_data['t'], result.height, result.t, f"{key_path}.t")

        if 'frames' in json_data:
            result.set_frames(list(map(lambda x: _resolve_tex(x, relative_to), json_data['frames'])))

    return result

def _align_memory(value:int) -> int:
    return (value + 7) & ~7

def parse_material(filename: str):
    if not os.path.exists(filename):
        raise Exception(f"The file {filename} does not exist")

    with open(filename, 'r') as json_file:
        json_data = json.load(json_file)

    result = Material()

    result.tex0 = _parse_tex(json_data['tex0'], 'tex0', filename) if 'tex0' in json_data else None
    result.tex1 = _parse_tex(json_data['tex1'], 'tex1', filename) if 'tex1' in json_data else None

    result.layout_textures()

    _parse_combine_mode(result, json_data)
    _parse_blend_mode(result, json_data)
    result.env_color = _parse_color(json_data['envColor'], 'envColor') if 'envColor' in json_data else None
    result.prim_color = _parse_color(json_data['primColor'], 'primColor') if 'primColor' in json_data else None
    result.blend_color = _parse_color(json_data['blendColor'], 'blendColor') if 'blendColor' in json_data else None

    result.lighting = json_data['lighting'] if 'lighting' in json_data else None

    result.culling = _optional_boolean(json_data, 'culling', 'culling', True)
    result.z_buffer = _optional_boolean(json_data, 'zBuffer', 'zBuffer', True)

    result.vtx_effect = _optional_string(json_data, 'uvGen', 'uvGen', 'none')

    result.fog = Fog()

    return result
