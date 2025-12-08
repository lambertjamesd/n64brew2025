const fs = require('fs');

const call_pairings = [
    ['malloc', 'free'],
    ['material_load', 'material_release'],
    [['material_cache_load', 'material_cache_load_from_file'], 'material_cache_release'],
    [
        ['render_scene_add', 'render_scene_add_renderable', 'render_scene_add_renderable'], 
        'render_scene_remove'
    ],
    [['renderable_init', 'renderable_single_axis_init'], 'renderable_destroy'],
    ['update_add', 'update_remove'],
    ['collision_scene_add', 'collision_scene_remove'],
    ['collision_scene_add_trigger', 'collision_scene_remove_trigger'],
    ['animator_init', 'animator_destroy'],
    ['animation_cache_load', 'animation_cache_release'],
    ['spell_exec_init', 'spell_exec_destroy'],
    ['effect_malloc', 'effect_free'],
    ['rspq_block_end', 'rspq_block_free'],
    ['health_init', 'health_destroy'],
    [['cutscene_load', 'cutscene_new', 'cutscene_builder_finish'], ['cutscene_free', 'cutscene_runner_free_on_finish']],
    ['tmesh_cache_load', 'tmesh_cache_release'],
    ['static_particles_load', 'static_particles_release'],
];

function populate_parings(mapping, curr, value) {
    if (Array.isArray(curr)) {
        curr.forEach((entry) => populate_parings(mapping, entry, value));
    } else {
        mapping.set(curr, value);
    }
}

const pair_mapping = call_pairings.reduce((result, curr, index) => {
    populate_parings(result, curr[0], {index, offset: 1});
    populate_parings(result, curr[1], {index, offset: -1});
    return result;
}, new Map());

const token_search = /(\w+)[\s\n]*\(/gim;

function search_file(contents) {
    let match = token_search.exec(contents);

    const result = [];

    while (result.length < call_pairings.length) {
        result.push({score: 0, match_locations: []});
    }

    while (match) {
        const mapping = pair_mapping.get(match[1]);

        if (mapping) {
            const result_entry = result[mapping.index];
            result_entry.score += mapping.offset;
            result_entry.match_locations.push({
                match: match[1],
                index: match.index
            });
        }
        
        match = token_search.exec(contents);
    }

    return result.filter((entry) => entry.score != 0);
}

function find_line(contents, location) {
    let line = 1;
    let col = 1;

    for (let i = 0; i < location; i += 1) {
        if (contents[i] == '\n') {
            line += 1;
            col = 1;
        } else {
            col += 1;
        }
    }

    return {line, col};
}

function extract_source(contents, location) {
    const before = contents.lastIndexOf('\n', location);
    const after = contents.indexOf('\n', location);

    return contents.substring(before + 1, after === -1 ? contents.length : after);
}

function count_occurence(match_locations) {
    const count = new Map();

    match_locations.forEach((entry) => {
        count.set(entry.match, (count.get(entry.match) || 0) + 1);
    });

    return Array.from(count.entries()).map((entry) => `${entry[0]} = ${entry[1]}`).join(', ');
}

function process_file(filename) {
    return new Promise((resolve) => {
        fs.readFile(filename, 'utf8', (err, data) => {
            if (err) {
                console.error(`failed to open file ${filename}: ${err.message}`);
                resolve(false);
                return;
            }

            const bad_pairings = search_file(data);

            if (bad_pairings.length == 0) {
                resolve(true);
                return;
            }

            console.error(`file ${filename} has mismatched pairings
${bad_pairings.map((pairing) => '    pairings ' + count_occurence(pairing.match_locations) + '\n' + pairing.match_locations.map((match) => {
    const location = find_line(data, match.index);
    return `        ${match.match} ${filename}:${location.line}\n            ${extract_source(data, match.index)}`
}).join('\n')).join('\n')}`);

            resolve(false);
        });
    });
}

function process_files(files) {
    return Promise.all(files.map(process_file)).then((results) => results.every(a => a));
}

process_files(process.argv.slice(2)).then((result) => {
    if (!result) {
        process.exit(1);
    } else {
        console.log('Good to go!');
    }
});