import struct
import io
import sys

from . import parser
from . import expresion_generator
from . import variable_layout
from . import static_evaluator

CUTSCENE_STEP_DIALOG = 0
CUTSCENE_STEP_SHOW_ITEM = 1
CUTSCENE_STEP_PAUSE = 2
CUTSCENE_STEP_EXPRESSION = 3
CUTSCENE_STEP_JUMP_IF_NOT = 4
CUTSCENE_STEP_JUMP = 5
CUTSCENE_STEP_SET_LOCAL = 6
CUTSCENE_STEP_SET_SCENE = 7
CUTSCENE_STEP_SET_GLOBAL = 8
CUTSCENE_STEP_DELAY = 9
CUTSCENE_STEP_INTERACT_WITH_NPC = 10
CUTSCENE_STEP_IDLE_NPC = 11
CUTSCENE_STEP_CAMERA_LOOK_AT_NPC = 12
CUTSCENE_STEP_CAMERA_FOLLOW = 13
CUTSCENE_STEP_CAMERA_RETURN = 14
CUTSCENE_STEP_CAMERA_ANIMATE = 15
CUTSCENE_STEP_CAMERA_MOVE_TO = 16
CUTSCENE_STEP_CAMERA_WAIT = 17
CUTSCENE_STEP_INTERACT_WITH_LOCATION = 18
CUTSCENE_STEP_FADE = 19
CUTSCENE_STEP_INTERACT_WITH_POSITION = 20
CUTSCENE_STEP_NPC_WAIT = 21
CUTSCENE_STEP_NPC_SET_SPEED = 22
CUTSCENE_STEP_SHOW_TITLE = 23
CUTSCENE_STEP_LOOK_AT_SUBJECT = 24
CUTSCENE_STEP_NPC_ANIMATE = 25
CUTSCENE_STEP_PRINT = 26
CUTSCENE_STEP_SPAWN = 27
CUTSCENE_STEP_CALLBACK = 28

class ParameterType():
    def __init__(self, name: str, is_static: bool):
        self.name: str = name
        self.is_static: bool = is_static

class Alias():
    def __init__(self, source: str, parameters: list[ParameterType] = []):
        self.source: str = source
        self.parameters: list[ParameterType] = parameters

_step_args = {
    "say": [ParameterType("tstr", True)],
    "pause": [ParameterType("bool", True), ParameterType("bool", True)],
    "delay": [ParameterType("float", True)],
    "interact_with_npc": [ParameterType("int", True), ParameterType("entity_id", False), ParameterType("entity_id", False)],
    "idle_npc": [ParameterType("entity_id", False)],
    "cam_look_npc": [ParameterType("entity_id", False)],
    "cam_follow": [],
    "cam_return": [],
    "cam_animate": [ParameterType("str", True)],
    "cam_wait": [],
    "interact_with_location": [ParameterType("int", True), ParameterType("entity_id", False), ParameterType("str", True)],
    "fade": [ParameterType("int", True), ParameterType("float", True)],
    "interact_with_position": [
        ParameterType("int", True), 
        ParameterType("entity_id", False), 
        ParameterType("float", True), 
        ParameterType("float", True), 
        ParameterType("float", True)
    ],
    "npc_wait": [ParameterType("entity_id", False)],
    "npc_set_speed": [ParameterType("entity_id", False), ParameterType("float", True)],
    "show_title": [ParameterType("str", True)],
    "look_at_subject": [],
    "npc_animate": [ParameterType("entity_id", False), ParameterType("str", True), ParameterType("bool", True)],
    "print": [ParameterType("tstr", True)],
    "spawn": [ParameterType("entity_spawner", False)],
}

_step_ids = {
    "say": CUTSCENE_STEP_DIALOG,
    "pause": CUTSCENE_STEP_PAUSE,
    "delay": CUTSCENE_STEP_DELAY,
    "interact_with_npc": CUTSCENE_STEP_INTERACT_WITH_NPC,
    "idle_npc": CUTSCENE_STEP_IDLE_NPC,
    "cam_look_npc": CUTSCENE_STEP_CAMERA_LOOK_AT_NPC,
    "cam_follow": CUTSCENE_STEP_CAMERA_FOLLOW,
    "cam_return": CUTSCENE_STEP_CAMERA_RETURN,
    "cam_animate": CUTSCENE_STEP_CAMERA_ANIMATE,
    "cam_wait": CUTSCENE_STEP_CAMERA_WAIT,
    "interact_with_location": CUTSCENE_STEP_INTERACT_WITH_LOCATION,
    "fade": CUTSCENE_STEP_FADE,
    "interact_with_position": CUTSCENE_STEP_INTERACT_WITH_POSITION,
    "npc_wait": CUTSCENE_STEP_NPC_WAIT,
    "npc_set_speed": CUTSCENE_STEP_NPC_SET_SPEED,
    "show_title": CUTSCENE_STEP_SHOW_TITLE,
    "look_at_subject": CUTSCENE_STEP_LOOK_AT_SUBJECT,
    "npc_animate": CUTSCENE_STEP_NPC_ANIMATE,
    "print": CUTSCENE_STEP_PRINT,
    "spawn": CUTSCENE_STEP_SPAWN,
}

_steps_that_need_idle = {
    "interact_with_npc",
    "interact_with_location",
    "interact_with_position",
}

_aliases: dict[str, Alias] = {
    "sign_start": Alias("pause true, false; look_at_subject;"),
    "sign_end": Alias("pause false, false;"),
    "world_pause": Alias("pause true, false;"),
    "world_unpause": Alias("pause false, false;"),
}

def _encode_string(string: str) -> bytes:
    encoded = string.encode()
    length = len(encoded)

    if length < 128:
        return struct.pack('>B', length) + encoded
    else:
        return struct.pack('>BB', (length & 127) | 128, length // 128) + encoded

class CutsceneStep():
    def __init__(self, command: int, data: bytes):
        self.command: int = command
        self.data: bytes = data

class JumpCutsceneStep():
    def __init__(self, command: int, label: str | None = None, token = None):
        self.command: int = command
        self.offset: int = 0
        self.label: str | None = label
        self.token = token

class Cutscene():
    def __init__(self):
        self.steps: list[CutsceneStep | JumpCutsceneStep] = []
        self.labels: dict[str, int] = {}

    def add_label(self, label: str):
        self.labels[label] = len(self.steps)

    def apply_jump_labels(self) -> list[str]:
        errors = []

        for idx, step in enumerate(self.steps):
            if not isinstance(step, JumpCutsceneStep):
                continue
            if not step.label:
                continue

            if not step.label in self.labels:
                errors.append(step.token.format_message(f'the label {step.label} was not found'))

            step.offset = self.labels[step.label] - (idx + 1)

        return errors
            
def build_template_string(string: parser.String, context: variable_layout.VariableContext):
    parts: list[str] = []

    for idx, replacement in enumerate(string.replacements):
        parts.append(string.contents[idx])

        type_checker = expresion_generator.TypeChecker(context)
        argument_type = type_checker.determine_type(replacement)

        if argument_type == 'str':
            parts.append('%s')
        if argument_type == 'int':
            parts.append('%d')
        if argument_type == 'float':
            parts.append('%f')

    parts.append(string.contents[-1])
        
    return ''.join(parts)


def _generate_function_step(cutscene: Cutscene, step: parser.CutsceneStep, args: list[ParameterType], context:variable_layout.VariableContext):
    pre_expression: expresion_generator.ExpressionScript | None = None

    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            if not isinstance(parameter, parser.String):
                raise Exception('Parameter should be a string')

            for replacment in parameter.replacements:
                expression = expresion_generator.generate_script(replacment, context)
                if not expression:
                    raise Exception(f"Could not generate expression {replacment}")   

                if pre_expression:
                    pre_expression = pre_expression.concat(expression)
                else:
                    pre_expression = expression

        if arg.is_static:
            continue

        expression = None

        if arg.name == 'int' or arg.name == 'bool' or arg.name == 'entity_spawner' or arg.name == 'entity_id':
            expression = expresion_generator.generate_script(parameter, context, 'int')
        elif arg.name == 'float':
            expression = expresion_generator.generate_script(parameter, context, 'float')

        if not expression:
            raise Exception(f"Could not generate expression {parameter}")
        
        if pre_expression:
            pre_expression = pre_expression.concat(expression)
        else:
            pre_expression = expression
                
    if pre_expression:
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_EXPRESSION, pre_expression.to_bytes()))


    data = io.BytesIO()
    
    for idx, arg in enumerate(args):
        parameter = step.parameters[idx]

        if arg.name == 'tstr':
            data.write(struct.pack('>B', len(parameter.replacements)))
            data.write(_encode_string(build_template_string(parameter, context)))
            continue
            
        if arg.name == 'str':
            data.write(_encode_string(parameter.contents[0]))
            continue

        if not arg.is_static:
            continue

        eval = static_evaluator.StaticEvaluator()
        static_value = eval.check_for_literals(parameter)

        if arg.name == 'bool':
            data.write(struct.pack('>B', 1 if static_value else 0))
        elif arg.name == 'int':
            data.write(struct.pack('>i', int(static_value)))
        elif arg.name == 'entity_id':
            data.write(struct.pack('>H', int(static_value)))
        elif arg.name == 'float':
            data.write(struct.pack('>f', float(static_value)))
        else:
            raise Exception(f"could not write arg of type {arg.name}")

    cutscene.steps.append(CutsceneStep(_step_ids[step.name.value], data.getvalue()))

def _generate_alias(cutscene: Cutscene, step: parser.CutsceneStep, alias: Alias, context:variable_layout.VariableContext, return_label: str):
    cutscene_alias = parser.parse(alias.source, "alias")

    for step in cutscene_alias.statements:
        _generate_step(cutscene, step, context, return_label)

def _validate_step(step, errors: list[str], context: variable_layout.VariableContext):
    if isinstance(step, parser.CutsceneStep):
        if step.name.value == 'return':
            return
        
        args = None

        if step.name.value in _step_args:
            args = _step_args[step.name.value]
        elif step.name.value in _aliases:
            args = _aliases[step.name.value].parameters

        if args == None:
            errors.append(step.name.format_message(f'{step.name.value} is not a valid step name'))
            return

        if len(args) != len(step.parameters):
            errors.append(step.name.format_message(f'incorrect number of parameters got {len(step.parameters)} expected {len(args)}'))
        
        for i in range(min(len(args), len(step.parameters))):
            parameter = step.parameters[i]
            arg_type = args[i]

            type_info = expresion_generator.TypeChecker(context)
            parameter_type = type_info.determine_type(parameter)

            errors += type_info.errors

            if arg_type.name == 'tstr':
                if not isinstance(parameter, parser.String):
                    errors.append(parameter.at.format_message('expected string'))
                else:
                    for replacement in parameter.replacements:
                        replacement_type = type_info.determine_type(replacement)
                        
                        if replacement_type != 'str' and replacement_type != 'int' and replacement_type != 'float':
                            errors.append(replacement.at.format_message(f'expected string, int or float but got {replacement_type}'))
                            
                        
                continue
            elif arg_type.name == 'str':
                if isinstance(parameter, parser.String):
                    if len(parameter.replacements):
                        errors.append(parameter.at.format_message('template parameters not allowed for basic strings'))
                else:
                    errors.append(parameter.at.format_message('expected string for parameter'))
                continue
            
            if arg_type.name == 'bool' or arg_type.name == 'int' or arg_type.name == 'entity_id':
                if parameter_type != 'int':
                    errors.append(parameter.at.format_message(f'expected int got {parameter_type}'))
            elif arg_type.name == 'float':
                if parameter_type != 'float' and parameter_type != 'int':
                    errors.append(parameter.at.format_message(f'expected float got {parameter_type}'))
            else:
                raise Exception(f'unknown type {arg_type.name}')
            
            if arg_type.is_static:
                eval = static_evaluator.StaticEvaluator()
                static_value = eval.check_for_literals(parameter)

                if static_value == None:
                    errors.append(parameter.at.format_message(f'parameter must be a literal'))
            
    if isinstance(step, parser.IfStatement):
        type_info = expresion_generator.TypeChecker(context)
        condition_type = type_info.determine_type(step.condition)
        errors += type_info.errors

        if not expresion_generator.is_numerical_type(condition_type):
            errors.append('condition should be an int or float')

        validate_steps(step.statements, errors, context)

    if isinstance(step, parser.Assignment):
        type_info = expresion_generator.TypeChecker(context)
        value_type = type_info.determine_type(step.value)
        errors += type_info.errors

        target_type = context.get_variable_type(step.name.value)

        if not expresion_generator.is_numerical_type(value_type):
            errors.append(step.name.format_message(f'can only assign numerical types'))

        if not target_type:
            errors.append(step.name.format_message('variable not defined'))
            return
        
        if not target_type in expresion_generator.type_mapping:
            errors.append(step.name.format_message(f'cannot assign type {target_type}'))

def validate_steps(statements: list, errors: list[str], context: variable_layout.VariableContext):
    for statement in statements:
        _validate_step(statement, errors, context)

def _generate_step(cutscene: Cutscene, step, context: variable_layout.VariableContext, return_label: str):
    if isinstance(step, parser.CutsceneStep):
        if step.name.value == 'return':
            cutscene.steps.append(JumpCutsceneStep(CUTSCENE_STEP_JUMP, return_label, step.name))
            return
        
        if step.name.value in _aliases:
            _generate_alias(cutscene, step, _aliases[step.name.value], context, return_label)
            return

        _generate_function_step(cutscene, step, _step_args[step.name.value], context)

    if isinstance(step, parser.IfStatement):
        expression = expresion_generator.generate_script(step.condition, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.condition}")
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_EXPRESSION, expression.to_bytes()))

        if_step = JumpCutsceneStep(CUTSCENE_STEP_JUMP_IF_NOT)
        cutscene.steps.append(if_step)
        size_before = len(cutscene.steps)
        
        for statement in step.statements:
            _generate_step(cutscene, statement, context, return_label)

        if step.else_block:
            # setup the jump that skips over the else block
            # after the end of the if block
            skip_else = JumpCutsceneStep(CUTSCENE_STEP_JUMP)
            cutscene.steps.append(skip_else)

            # if there is an else block, skipping the if content should 
            # jump to the start of the else block
            if_step.offset = len(cutscene.steps) - size_before

            size_before = len(cutscene.steps)

            for statement in step.else_block:
                _generate_step(cutscene, statement, context, return_label)

            # update the correct jump offset
            skip_else.offset = len(cutscene.steps) - size_before
        else:
            # update the correct jump offset
            if_step.offset = len(cutscene.steps) - size_before

    if isinstance(step, parser.Assignment):
        expression = expresion_generator.generate_script(step.value, context)
        if not expression:
            raise Exception(f"Could not generate expression {step.value}")
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_EXPRESSION, expression.to_bytes()))
        
        step_type = CUTSCENE_STEP_SET_LOCAL

        if context.is_global(step.name.value):
            step_type = CUTSCENE_STEP_SET_GLOBAL
        elif context.is_scene_var(step.name.value):
            step_type = CUTSCENE_STEP_SET_SCENE

        var_type = context.get_variable_type(step.name.value)
        bit_offset = context.get_variable_offset(step.name.value)

        cutscene.steps.append(CutsceneStep(
            step_type, 
            expresion_generator.generate_variable_address(var_type, bit_offset)
        ))

def _idle_find_effected_actors(statements: list, actors: set):
    for statement in statements:
        if isinstance(statement, parser.IfStatement):
            _idle_find_effected_actors(statement.statements, actors)

        if isinstance(statement, parser.CutsceneStep) and statement.name.value in _steps_that_need_idle:
            actors.add(static_evaluator.StaticEvaluator().check_for_literals(statement.parameters[1]))

def _idle_effected_actors(cutscene: Cutscene, statements: list):
    actors = set[int]()

    _idle_find_effected_actors(statements, actors)

    for actor in actors:
        expression = expresion_generator.ExpressionScript([
            expresion_generator.ExpresionScriptIntLiteral(actor),
            expresion_generator.ExpressionCommand(expresion_generator.EXPRESSION_TYPE_END),
        ])
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_EXPRESSION, expression.to_bytes()))
        cutscene.steps.append(CutsceneStep(CUTSCENE_STEP_IDLE_NPC, b''))

def _generate_statement_list_steps(cutscene: Cutscene, statements: list, context: variable_layout.VariableContext, function_name: str):
    for statement in statements:
        _generate_step(cutscene, statement, context, f'$return_{function_name}')

    cutscene.add_label(f'$return_{function_name}')

    _idle_effected_actors(cutscene, statements)

def generate_steps(file, inputCutscene: parser.Cutscene, context: variable_layout.VariableContext):
    cutscene = Cutscene()

    _generate_statement_list_steps(cutscene, inputCutscene.statements, context, '')

    fn_length: list[int] = []
    main_length = len(cutscene.steps)

    for fn in inputCutscene.functions:
        before = len(cutscene.steps)
        _generate_statement_list_steps(cutscene, fn.body, context, fn.name.value)
        fn_length.append(len(cutscene.steps) - before)

    errors = cutscene.apply_jump_labels()

    if len(errors) > 0:
        message = '\n\n'.join(errors)
        print(message)
        raise Exception(message)

    file.write('CTSN'.encode())

    file.write(struct.pack('>HH', len(cutscene.steps), 1 + len(inputCutscene.functions)))

    context.locals.write_default_values(file)

    for step in cutscene.steps:
        file.write(struct.pack('>B', step.command))
        if isinstance(step, CutsceneStep):
            file.write(step.data)
        elif isinstance(step, JumpCutsceneStep):
            file.write(struct.pack('>h', step.offset))

    file.write(struct.pack('>HB', main_length, 0))

    for length, fn in zip(fn_length, inputCutscene.functions):
        name_bytes = fn.name.value.encode()

        file.write(struct.pack('>HB', length, len(name_bytes)))
        file.write(name_bytes)

