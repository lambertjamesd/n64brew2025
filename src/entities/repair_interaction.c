#include "repair_interaction.h"

#include "../collision/shapes/box.h"
#include "../collision/shapes/cylinder.h"
#include "../collision/collision_scene.h"
#include "../render/render_scene.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"
#include "../time/time.h"
#include "../cutscene/expression_evaluate.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

struct repair_interaction_type_def {
    const char* mesh_name;
    const char* repaired_mesh_name;
    const char* repaired_mesh_anim;
    dynamic_object_type_t collider;
};

typedef struct repair_interaction_type_def repair_interaction_type_def_t;

static repair_interaction_type_def_t types[REPAIR_COUNT] = {
    [REPAIR_BIKE_MOTOR] = {
        .mesh_name = "rom:/meshes/repairs/hoverbike_no_motor.tmesh",
        .collider = {
            BOX_COLLIDER(0.313f, 0.3f, 0.9f),
            .center = {0.0f, 0.3f, 0.0f},
        }
    },
    [REPAIR_WELL] = {
        .mesh_name = "rom:/meshes/repairs/water_pump_broken.tmesh",
        .repaired_mesh_name = "rom:/meshes/repairs/water_pump_fixed.tmesh",
        .repaired_mesh_anim = "rom:/meshes/repairs/water_pump_fixed.anim",
        .collider = {
            BOX_COLLIDER(0.9f, 1.6f, 1.0f),
            .center = {0.0f, 0.8f, 0.0f},
        }
    },
    [REPAIR_GENERATOR] = {
        .mesh_name = "rom:/meshes/repairs/generator_broken.tmesh",
        .repaired_mesh_name = "rom:/meshes/repairs/generator_fixed.tmesh",
        .collider = {
            BOX_COLLIDER(0.6f, 0.6f, 1.0f),
            .center = {0.0f, 0.6f, 0.0f},
        }
    },
    [REPAIR_MEDICAL_DEVICE] = {
        .mesh_name = "rom:/meshes/repairs/medical_machine_broken.tmesh",
        .repaired_mesh_name = "rom:/meshes/repairs/medical_machine_broken.tmesh",
        .collider = {
            CYLINDER_COLLIDER(0.5f, 0.5f),
            .center = {0.0f, 0.5f, 0.0f},
        }
    },
    [REPAIR_TABLET] = {
        .mesh_name = "rom:/meshes/repairs/tablet_broken.tmesh",
        .collider = {
            BOX_COLLIDER(0.5f, 0.1f, 0.5f),
            .center = {0.0f, 0.1f, 0.0f},
        }
    },
};

void repair_interaction_common_init() {

}

void repair_interaction_common_destroy() {

}

void repair_interact(struct interactable* interactable, entity_id from) {
    repair_interaction_t* repair = (repair_interaction_t*)interactable->data;
    cutscene_builder_t builder;
    cutscene_builder_init(&builder);

    cutscene_builder_fade(&builder, FADE_COLOR_BLACK, 1.0f);
    cutscene_builder_pause(&builder, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_load_scene(&builder, repair->repair_scene);
    cutscene_builder_fade(&builder, FADE_COLOR_NONE, 1.0f);
    cutscene_builder_delay(&builder, 1.0f);
    cutscene_builder_pause(&builder, false, false, UPDATE_LAYER_WORLD);

    cutscene_runner_run(cutscene_builder_finish(&builder), 0, cutscene_runner_free_on_finish(), NULL, 0);
}

void repair_interaction_update(void *data) {
    repair_interaction_t* repair = (repair_interaction_t*)data;
    armature_t* armature = renderable_get_armature(&repair->renderable);
    animator_update(&repair->animator, armature, scaled_time_step);
}

void repair_interaction_init(repair_interaction_t* repair, struct repair_interaction_definition* definition, entity_id entity_id) {
    repair_interaction_type_def_t* def = &types[definition->repair_type];

    transformSaInit(&repair->transform, &definition->position, &definition->rotation, 1.0f);

    repair->is_repaired = expression_get_bool(definition->is_repaired);
    
    renderable_single_axis_init(&repair->renderable, &repair->transform, repair->is_repaired ? def->repaired_mesh_name : def->mesh_name);

    if (repair->is_repaired && def->repaired_mesh_anim) {
        armature_t* armature = renderable_get_armature(&repair->renderable);
        animator_init(&repair->animator, armature->bone_count);
        repair->animations = animation_cache_load(def->repaired_mesh_anim);
        animator_run_clip(&repair->animator, animation_set_find_clip(repair->animations, "idle"), 0.0f, true);
        update_add(repair, repair_interaction_update, UPDATE_PRIORITY_EFFECTS, UPDATE_LAYER_WORLD);
    } else {
        repair->animations = NULL;
    }

    render_scene_add_renderable(&repair->renderable, 2.0f);

    dynamic_object_init(entity_id, &repair->collider, &def->collider, COLLISION_LAYER_TANGIBLE, &repair->transform.position, &repair->transform.rotation);
    repair->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;
    repair->collider.is_fixed = true;
    collision_scene_add(&repair->collider);

    repair->repair_type = definition->repair_type;
    repair->repair_scene = definition->repair_scene;

    if (!repair->is_repaired) {
        interactable_init(&repair->interactable, entity_id, INTERACT_TYPE_REPAIR, repair_interact, repair);
    }
}

void repair_interaction_destroy(repair_interaction_t* repair) {
    render_scene_remove(&repair->renderable);
    renderable_destroy(&repair->renderable);
    collision_scene_remove(&repair->collider);

    if (repair->animations) {
        animator_destroy(&repair->animator);
        animation_cache_release(repair->animations);
        update_remove(repair);
    } 
    
    if (!repair->is_repaired) {
        interactable_destroy(&repair->interactable);
    }
}