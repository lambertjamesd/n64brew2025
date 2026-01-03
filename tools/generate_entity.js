const fs = require('fs');
const path = require('path');

const scene_definition = 'src/scene/scene_definition.h';
const entity_spawner = 'src/entity/entity_spawner.c';
const entities_location = 'src/entities';

function generate_header_file(name) {
    const include_guard = `__ENTITIES_${name.toUpperCase()}_H__`;
    return `#ifndef ${include_guard}
#define ${include_guard}

#include "../math/vector3.h"
#include "../entity/entity_id.h"
#include "../scene/scene_definition.h"

struct ${name} {
    vector3_t position;
};

typedef struct ${name} ${name}_t;

void ${name}_init(${name}_t* ${name}, struct ${name}_definition* definition, entity_id entity_id);
void ${name}_destroy(${name}_t* ${name}, struct ${name}_definition* definition);
void ${name}_common_init();
void ${name}_common_destroy();

#endif`;
}

function generate_c_file(name) {
    return `#include "${name}.h"    
    
void ${name}_init(${name}_t* ${name}, struct ${name}_definition* definition, entity_id entity_id) {
    ${name}->position = definition->position;
}

void ${name}_destroy(${name}_t* ${name}, struct ${name}_definition* definition) {
    
}

void ${name}_common_init() {

}

void ${name}_common_destroy() {

}
`
}

function generate_definition(name) {
    return `struct ${name}_definition {
    struct Vector3 position;    
};
`
}

function generate_entity_entry(name) {
    return `ENTITY_DEFINITION(${name}, fields_empty),`;
}

function generate_include(name) {
    return `#include "../entities/${name}.h"`
}

function generate_type_enum(name) {
    return `ENTITY_TYPE_${name},`;
}

const IS_SPACE = /[ \t]/;

function get_indent(contents, at) {
    let start = at;

    while (start > 1 && IS_SPACE.test(contents[start-1])) {
        --start;
    }

    return contents.substring(start, at);
}

function apply_edit(contents, search, replacement, checker) {
    if (contents.indexOf(checker || replacement) !== -1) {
        return contents;
    }

    const insertPoint = contents.indexOf(search);

    if (insertPoint === -1) {
        return contents;
    }

    return contents.substring(0, insertPoint) + replacement + '\n' + get_indent(contents, insertPoint) + contents.substring(insertPoint);
}

const entity_name = process.argv[2];

let entity_spawner_contents = fs.readFileSync(entity_spawner, 'utf-8');
entity_spawner_contents = apply_edit(
    entity_spawner_contents, 
    '// include_list insert point',
    generate_include(entity_name)
);

entity_spawner_contents = apply_edit(
    entity_spawner_contents, 
    '// scene_entity_definitions insert point',
    generate_entity_entry(entity_name)
);

fs.writeFileSync(entity_spawner, entity_spawner_contents)

let scene_definition_contents = fs.readFileSync(scene_definition, 'utf-8');
scene_definition_contents = apply_edit(
    scene_definition_contents,
    '// definition insert point',
    generate_definition(entity_name),
    `struct ${entity_name}_definition {`
);
scene_definition_contents = apply_edit(
    scene_definition_contents,
    '// type enum insert point',
    generate_type_enum(entity_name)
)

fs.writeFileSync(scene_definition, scene_definition_contents)

const c_file_name = `${entities_location}/${entity_name}.c`;
if (!fs.existsSync(c_file_name)) {
    fs.writeFileSync(c_file_name, generate_c_file(entity_name));
}

const h_file_name = `${entities_location}/${entity_name}.h`;
if (!fs.existsSync(h_file_name)) {
    fs.writeFileSync(h_file_name, generate_header_file(entity_name));
}