import sys
import argparse
import cutscene.tokenizer
import cutscene.parser
import cutscene.variable_layout

mapping = {
    "has_motor": 'ITEM_WELL_HAS_MOTOR',
    "has_repaired_motor": 'ITEM_WELL_HAS_FIXED_MOTOR',
    "has_well_part_map": 'ITEM_WELL_PUMP_PART_MAP',
    "has_pump_gear": 'ITEM_WELL_HAS_PUMP_GEAR',
    "has_repaired_pump": 'ITEM_WELL_HAS_FIXED_PUMP',
    
    "has_generator_part_map": 'ITEM_GENERATOR_PART_MAP',
    "has_generator_part_0": 'ITEM_GENERATOR_PART_0',
    "has_generator_part_1": 'ITEM_GENERATOR_PART_1',
    "has_generator_part_2": 'ITEM_GENERATOR_PART_2',
    "has_repaired_generator": 'ITEM_GENERATOR_HAS_FIXED',
}

data_type_mapping = {
    "i8": "DATA_TYPE_S8",
    "i16": "DATA_TYPE_S16",
    "i32": "DATA_TYPE_S32",
    "entity_id": "DATA_TYPE_S16",
    "entity_spawner": "DATA_TYPE_S32",
    "bool": "DATA_TYPE_BOOL",
    "float": "DATA_TYPE_F32",
    "str": "DATA_TYPE_ADDRESS",
}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        prog='Material Writer',
        description='Converts a json material into a material binary'
    )

    parser.add_argument('output')
    parser.add_argument('inventory_mapping')
    parser.add_argument('input', nargs='*')

    args = parser.parse_args()
    result = cutscene.variable_layout.VariableLayoutBuilder()

    success = True
    
    for input in args.input:
        with open(input) as file:
            current_cutscene = cutscene.parser.parse(file.read(), args.input)

            for global_var in current_cutscene.globals:
                success = result.add_variable(global_var) and success

    if not success:
        sys.exit(1)

    entries = result.build()

    with open(args.output + '.dat', 'wb') as file:
        entries.write_default_values(file)

    with open(args.output + '.json', 'w') as file:
        result.serialize(file)
    
    with open(args.inventory_mapping, 'w') as file:
        file.write('#include "inventory.h"\n')
        file.write('\n')
        file.write('struct global_location inventory_item_locations[ITEM_TYPE_COUNT] = {\n')

        for entry in entries.get_all_entries():
            if not entry.name in mapping:
                continue

            file.write('    [' + mapping[entry.name] + '] = { .data_type = ' + data_type_mapping[entry.type_name] + ', .word_offset = ' + str(entry.offset // entry.bit_size) + ' },\n')

        file.write('};')