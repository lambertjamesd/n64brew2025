import sys

from . import parser

sys.path.append("..")

import parse.struct_parse
    
global_constant_values: dict[str, int] = {
    'true': 1,
    'false': 0,
}

with open('src/scene/scene_definition.h', 'r') as file:
    file_content = file.read()
    enums = parse.struct_parse.find_enums(file_content)

    for value in enums.values():
        value.populate_dict(global_constant_values)

def type_from_pytype(pytype):
    if isinstance(pytype, int):
        return 'int'
    if isinstance(pytype, float):
        return 'float'
    if isinstance(pytype, str):
        return 'str'
    
    return 'error'

class StaticEvaluator():
    def __init__(self):
        self.literal_value = {}

    def _check_for_literals(self, expression: parser.Expression):
        if isinstance(expression, parser.Integer):
            return int(expression.value.value)
        if isinstance(expression, parser.Float):
            return float(expression.value.value)
        if isinstance(expression, parser.String):
            return None
        if isinstance(expression, parser.UnaryOperator):
            operand_value = self.check_for_literals(expression.operand)

            if operand_value is None:
                return None

            if expression.operator.value == '-':
                return -operand_value

            if expression.operator.value == 'not':
                return 0 if operand_value else 1
        if isinstance(expression, parser.BinaryOperator):
            a_value = self.check_for_literals(expression.a)
            b_value = self.check_for_literals(expression.b)

            if a_value is None or b_value is None:
                return None
            
            if expression.operator.value == 'and':
                return 1 if a_value and b_value else 0
            if expression.operator.value == 'or':
                return 1 if a_value or b_value else 0
            if expression.operator.value == '==':
                return 1 if a_value == b_value else 0
            if expression.operator.value == '!=':
                return 1 if a_value != b_value else 0
            if expression.operator.value == '>':
                return 1 if a_value > b_value else 0
            if expression.operator.value == '<':
                return 1 if a_value < b_value else 0
            if expression.operator.value == '>=':
                return 1 if a_value >= b_value else 0
            if expression.operator.value == '<=':
                return 1 if a_value <= b_value else 0
            if expression.operator.value == '+':
                return a_value + b_value
            if expression.operator.value == '-':
                return a_value - b_value
            if expression.operator.value == '*':
                return a_value * b_value
            if expression.operator.value == '/':
                if type_from_pytype(a_value) == 'int' and type_from_pytype(b_value) == 'int':
                    return a_value // b_value
                return a_value / b_value
        if isinstance(expression, parser.Identifier):
            if expression.name.value in global_constant_values:
                return global_constant_values[expression.name.value]
        if isinstance(expression, parser.FunctionCall):
            for arg in expression.args:
                self.check_for_literals(arg)
        
        return None

    def check_for_literals(self, expression: parser.Expression):
        result = self._check_for_literals(expression)
        self.literal_value[expression] = result
        return result