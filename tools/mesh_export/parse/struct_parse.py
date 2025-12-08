
class StructureAttribute:
    def __init__(self, name: str, data_type: str):
        self.name: str = name
        self.data_type: str = data_type
    
    def __str__(self):
        return f"{self.name}: {self.data_type}"

class StructureInfo:
    def __init__(self, name: str, children: list[StructureAttribute]):
        self.name: str = name
        self.children: list[StructureAttribute] = children
        self.align = 0

    def __str__(self):
        result: list[str] = []

        result.append(f"struct {self.name} {'{'}")

        for child in self.children:
            result.append(f"    {str(child)}")

        result.append("}")

        return '\n'.join(result)
    
class PointerType:
    def __init__(self, sub_type):
        self.sub_type = sub_type

class EnumValue:
    def __init__(self, name: str, value: int):
        self.name: str = name
        self.value: int = value

class EnumInfo:
    def __init__(self, name: str, values: list[EnumValue]):
        self.name: str = name
        self._values: list[EnumValue] = values

    def str_to_int(self, value: str) -> int | None:
        for entry in self._values:
            if entry.name == value:
                return entry.value
            
        return None
    
    def int_to_str(self, value: int) -> str | None:
        for entry in self._values:
            if entry.value == value:
                return entry.name
            
        return None
    
    def all_values(self) -> list[EnumValue]:
        return self._values
    
    def is_defined(self, name: str) -> bool:
        for entry in self._values:
            if entry.name == name:
                return True
        return False
    
    def populate_dict(self, dict: dict[str, int]):
        for entry in self._values:
            dict[entry.name] = entry.value

class UnorderedEnum:
    def __init__(self, name: str, values: dict[str, int]):
        self.name = name
        self._values: dict[str, int] = values

        flipped = {}

        for name, val in values.items():
            flipped[val] = name

        self._flipped: dict[int, str] = flipped

    def str_to_int(self, value: str):
        return self._values[value]
    
    def int_to_str(self, value: int):
        return self._flipped[value]
    
    def all_values(self) -> list[str]:
        return list(self._values.keys())
    
    def is_defined(self, name: str) -> bool:
        return name in self._values
    
    def populate_dict(self, dict: dict[str, int]):
        for name, idx in self._values.items():
            dict[name] = idx

class Token:
    def __init__(self, value: str, token_type: str, at: int):
        self.value: str = value
        self.token_type: str = token_type
        self.at: int = at

    def __str__(self):
        return f"{self.value} : {self.token_type} : {self.at}"

def token_identifier(chr: str):
    if chr.isalpha() or chr.isdigit() or chr == '_':
        return (token_identifier, None)
    
    return (token_default_state(chr), 'identifier')

def token_int(chr: str):
    if chr.isdigit():
        return (token_int, None)
    
    return (token_default_state(chr), 'int')

def token_emit(token_type: str):
    def result(chr: str):
        return (token_default_state(chr), token_type)
    
    return result

def token_white(chr: str):
    if chr.isspace():
        return (token_white, None)
    return (token_default_state(chr), 'white')

def token_single_comment(chr: str):
    if chr == '\n':
        return (token_white, 'comment')
    return (token_single_comment, None)

def token_multi_comment_2(chr: str):
    if chr == '/':
        return (token_emit('comment'), None)
    if chr == '*':
        return (token_multi_comment_2, None)
    
    return (token_multi_comment, None)

def token_multi_comment(chr: str):
    if chr == '*':
        return (token_multi_comment_2, None)
    
    return (token_multi_comment, None)

def token_comment_start(chr: str):
    if chr == '/':
        return (token_single_comment, None)
    if chr == '*':
        return (token_multi_comment, None)
    return (token_default_state, 'error')

def token_error(chr: str):
    if chr == '':
        return (token_error, 'error')
    
    return (token_error, None)

single_tokens = {'{', '}', ';', ',', '*', '='}

def token_default_state(chr: str):
    if chr.isalpha() or chr == '_':
        return token_identifier
    if chr.isdigit():
        return token_int
    if chr.isspace():
        return token_white
    if chr == '/':
        return token_comment_start
    if chr in single_tokens:
        return token_emit(chr)
    
    return token_error

def tokenize(string: str, offset: int) -> list[Token]:
    state = token_default_state(string[0])
    last_start = 0

    result: list[Token] = []

    for idx in range(1, len(string) + 1):
        if idx == len(string):
            chr = ''
        else:
            chr = string[idx]

        next, token_type = state(chr)

        state = next

        if token_type:
            if token_type != 'white' and token_type != 'comment':
                token = Token(string[last_start:idx], token_type, last_start + offset)
                result.append(token)
            last_start = idx

    result.append(Token('', 'eof', len(string) + offset))

    return result

def determine_source_location(source: str, offset: int):
    line = 1
    col = 1

    for idx in range(offset):
        if source[idx] == '\n':
            line += 1
            col = 1
        else:
            col += 1

    return (line, col)

def get_line_start(source: str, offset: int):
    return source.rfind('\n', 0, offset) + 1

def get_line_end(source: str, offset: int):
    result = source.find('\n', offset)

    if result == -1:
        return len(source)
    
    return result

def get_full_line(source: str, offset: int):
    start = get_line_start(source, offset)
    end = get_line_end(source, offset)
    return source[start:end]

class ParseState:
    def __init__(self, tokens: list[Token], source: str):
        self.tokens: list[Token] = tokens
        self.current: int = 0
        self.source: str = source

    def format_error(self, at: Token, message: str):
        line, col = determine_source_location(self.source, at.at)
        return f"{message}: line {line} col {col}\n{get_full_line(self.source, at.at)}\n{' ' * (col - 1)}^"

    def peek(self, offset: int):
        return self.tokens[min(offset + self.current, len(self.tokens) - 1)]
    
    def advance(self):
        self.current += 1
    
    def require(self, token_type, value = None):
        next = self.peek(0)

        if next.token_type != token_type:
            raise Exception(self.format_error(next, f"expected {token_type} got {next.token_type}"))
        
        if not value is None and next.value != value:
            raise Exception(self.format_error(next, f"expected {value} got {next.value}"))
        
        self.advance()

        return next
    
    def optional(self, token_type, value = None) -> Token | None:
        next = self.peek(0)

        if next.token_type != token_type:
            return None
        
        if not value is None and value != next.value:
            return None

        self.advance()
        return next

def parse_struct(state: ParseState):
    state.require('identifier', 'struct')

    name = state.optional('identifier')

    if name and state.peek(0).token_type != '{':
        return f"struct {name.value}"

    state.require('{')

    children: list[StructureAttribute] = []

    while not state.optional('}'):
        if state.peek(0).token_type == 'eof':
            raise Exception(state.format_error(state.peek(0), 'unexpected end of file'))
        
        data_type = _parse_type(state)
        field_name = state.require('identifier')
        state.require(';')

        children.append(StructureAttribute(field_name.value, data_type))

    return StructureInfo('' if name is None else name.value, children)

def parse_enum(state: ParseState):
    state.require('identifier', 'enum')
    
    name = state.optional('identifier')

    if name and state.peek(0).token_type != '{':
        return f"enum {name.value}"
    elif not name:
        raise Exception('enum needed a name')
    
    state.require('{')

    values: list[EnumValue] = []

    current_value: int = 0

    while not state.optional('}'):
        if state.peek(0).token_type == 'eof':
            raise Exception(state.format_error(state.peek(0), 'unexpected end of file'))
        
        value_name = state.require('identifier')

        if state.optional('='):
            current_value = int(state.require('int').value)

        values.append(EnumValue(value_name.value, current_value))
        state.optional(',')

        current_value += 1

    return EnumInfo(name.value, values)

def _parse_type_scalar(state: ParseState):
    next = state.peek(0)

    if next.token_type == 'identifier' and next.value == 'struct':
        return parse_struct(state)
    
    if next.token_type == 'identifier' and next.value == 'enum':
        return parse_enum(state)
    
    if next.token_type == 'identifier':
        state.advance()
        return next.value

    raise Exception(state.format_error(state.peek(0), 'unknown type'))


def _parse_type(state: ParseState):
    result = _parse_type_scalar(state)

    if state.optional('*'):
        return PointerType(result)
    
    return result

def determine_enum(source: str, starting_at: int, ending_at: int):
    struct_source = source[starting_at:ending_at]
    return parse_enum(ParseState(tokenize(struct_source, starting_at), source))


def determine_type(source: str, starting_at: int, ending_at: int):
    struct_source = source[starting_at:ending_at]
    return _parse_type(ParseState(tokenize(struct_source, starting_at), source))

def find_end_curly(file_string: str, starting_at: int):
    depth = 0

    for idx in range(starting_at, len(file_string)):
        if file_string[idx] == '{':
            depth += 1

        if file_string[idx] == '}':
            depth -= 1

            if depth == 0:
                return idx + 1

    return -1

def find_enums(file_string: str) -> dict[str, EnumInfo]:
    result: dict[str, EnumInfo] = {}
    
    current_position = file_string.find('enum ')

    result: dict[str, EnumInfo] = {}

    while current_position != -1:
        # only look for enums definitons
        # this is pretty shaky but should work
        if file_string[current_position - 1] != '\n':
            current_position = file_string.find('enum ', current_position + 5)
            continue

        next_end = find_end_curly(file_string, current_position)

        if next_end == -1:
            raise Exception('Unmatched curly braces')

        enum_type = determine_enum(file_string, current_position, next_end)

        result[f'enum {enum_type.name}'] = enum_type

        current_position = file_string.find('enum ', next_end)

    return result


def find_structs(file_string: str) -> dict[str, StructureInfo]:
    current_position = file_string.find('struct ')

    result: dict[str, StructureInfo] = {}

    while current_position != -1:
        next_end = find_end_curly(file_string, current_position)

        if next_end == -1:
            raise Exception('Unmatched curly braces')

        struct_type = determine_type(file_string, current_position, next_end)

        result[struct_type.name] = struct_type

        current_position = file_string.find('struct ', next_end)

    return result