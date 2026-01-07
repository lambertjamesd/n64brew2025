
EXPRESSION_BUILT_IN_ARE_TOUCHING = 0
EXPRESSION_BUILT_ASK_RESPONSE = 1

class FunctionParameter():
    def __init__(self, name: str, type: str):
        self.name: str = name
        self.type: str = type

class BuiltInFunction():
    def __init__(self, index: int, return_type: str, args: list[FunctionParameter]):
        self.index: int = index
        self.return_type: str = return_type
        self.args: list[FunctionParameter] = args

    def get_arg_type(self, index: int) -> str:
        if index < len(self.args):
            return self.args[index].type

        return 'error'


_built_in_functions = {
    "are_touching": BuiltInFunction(EXPRESSION_BUILT_IN_ARE_TOUCHING, "int", [FunctionParameter("a", "int"), FunctionParameter("b", "int")]),
    "ask_response": BuiltInFunction(EXPRESSION_BUILT_ASK_RESPONSE, "int", []),
}

def lookup(name: str) -> BuiltInFunction | None:
    if name in _built_in_functions:
        return _built_in_functions[name]
    
    return None