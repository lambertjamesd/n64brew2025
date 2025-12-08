import argparse
import cutscene.tokenizer
import cutscene.parser
import cutscene.variable_layout
import cutscene.expresion_generator
import cutscene.variable_layout

if __name__ == "__main__":
    result = cutscene.parser.parse("""
        local a: i32;

        if a * 3 < 10 then
            return;
        end      

        say "this is a {a} \\\" test";
                                   
    """, "example")

    globals_builder = cutscene.variable_layout.VariableLayoutBuilder()
    
    for global_var in result.globals:
        globals_builder.add_variable(global_var)

    locals_builder = cutscene.variable_layout.VariableLayoutBuilder()
    
    for local_var in result.locals:
        locals_builder.add_variable(local_var)

    scene_builder = cutscene.variable_layout.VariableLayoutBuilder()

    for scene_var in result.scene_vars:
        scene_builder.add_variable(scene_var)

    globals = globals_builder.build()
    locals = locals_builder.build()
    scene_vars = scene_builder.build()

    print(cutscene.expresion_generator.generate_script(
        result.statements[0].condition, 
        cutscene.variable_layout.VariableContext(globals, scene_vars, locals)
    ))