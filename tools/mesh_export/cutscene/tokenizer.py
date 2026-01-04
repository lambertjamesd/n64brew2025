
class Source():
    def __init__(self, content: str, filename: str):
        self.content: str = content
        self.filename: str = filename

    def get_source_line(self, at: int) -> str:
        line_start = self.content.rfind('\n', 0, at)
        line_end = self.content.find('\n', at)
        return self.content[line_start:line_end]

    def determine_source_location(self, at: int) -> tuple[int, int]:
        line = 1
        col = 1

        for idx in range(at):
            if self.content[idx] == '\n':
                line += 1
                col = 1
            else:
                col += 1

        return line, col

    def format_message(self, message: str, at: int):
        line, col = self.determine_source_location(at)
        padding = ' ' * (col - 1)

        return f'{self.filename}:{line}:{col} {message}\n{self.get_source_line(at)}\n{padding}^'

class Token():
    def __init__(self, token_type: str, value: str, at: int, source: Source | None):
        self.token_type: str = token_type
        self.value: str = value
        self.at: int = at
        self._source: Source | None = source

    def __str__(self):
        return f"'{self.value}':{self.token_type}"
    
    def format_message(self, message: str) -> str:
        if not self._source:
            return message

        return self._source.format_message(message, self.at)

def _whitespace_state(current: str):
    if current.isspace():
        return _whitespace_state, None
    return _default_state(current), 'whitespace'

def _float_state(current: str):
    if current.isdigit():
        return _float_state, None
    
    return _default_state(current), 'float'

def _integer_state(current: str):
    if current.isdigit():
        return _integer_state, None
    if current == '.':
        return _float_state, None
    
    return _default_state(current), 'int'

def _unknown_state(current: str):
    next_state = _default_state(current)

    if current == '' or next_state != _unknown_state:
        return next_state, 'unknown'

    return _unknown_state, None

def _identifier_state(current: str):
    if current.isalnum() or current == '_':
        return _identifier_state, None
    
    return _default_state(current), 'identifier'

def _emit_token_state(token_type: str):
    def result(current: str):
        return _default_state(current), token_type
    return result

def _assign_or_equal_state(current: str):
    if current == '=':
        return _emit_token_state('=='), None
    return _default_state(current), '='

def _greater_than_state(current: str):
    if current == '=':
        return _emit_token_state('>='), None
    return _default_state(current), '>'

def _less_than_state(current: str):
    if current == '=':
        return _emit_token_state('<='), None
    return _default_state(current), '<'

def _comment_state(current: str):
    return _whitespace_state, 'comment'

def _comment_start_state(current: str):
    if current == '/':
        return _comment_state, None
    return _default_state(current), '/'

_single_character_tokens = {
    '+', '-', '*', '\\',
    ':', '[', ']', '(', ')', '{', '}', ',',
    ';', '"',
}

def _default_state(current: str):
    if current.isspace():
        return _whitespace_state
    if current.isalpha() or current == '_':
        return _identifier_state
    if current in _single_character_tokens:
        return _emit_token_state(current)
    if current.isdigit():
        return _integer_state
    if current == '/':
        return _comment_start_state
    if current == '#':
        return _comment_state
    if current == '.':
        return _float_state
    if current == '=':
        return _assign_or_equal_state
    if current == '>':
        return _greater_than_state
    if current == '<':
        return _less_than_state
    
    return _unknown_state

def tokenize(content: str, filename: str) -> list[Token]:
    result: list[Token] = []
    state = _default_state(content[0])
    last_start = 0

    source = Source(content, filename)

    for idx in range(1, len(content) + 1):
        character = '' if idx == len(content) else content[idx]

        next_state, emit_token = state(character)

        if emit_token:
            result.append(Token(emit_token, content[last_start:idx], last_start, source))
            last_start = idx

        state = next_state

    result.append(Token('eof', '', len(content), source))

    return result