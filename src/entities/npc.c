#include "npc.h"

#include "../collision/shapes/capsule.h"
#include "../cutscene/cutscene_runner.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../time/time.h"

struct npc_information npc_information[] = {
    [NPC_TYPE_SCRAPBOT1] = {
        .mesh = "rom:/meshes/characters/NPC_scrapbot1.tmesh",
        .animations = "rom:/meshes/characters/NPC_scrapbot1.anim",
        .actor = {
            .eye_level = 1.81147f,
            .move_speed = 1.0f,
            .rotate_speed = 2.0f,
            .collision_layers = COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_Z_TARGET,
            .collider = {
                CAPSULE_COLLIDER(0.3f, 0.45f),
                // about a 40 degree slope
                .max_stable_slope = 0.219131191f,
                .friction = 0.5f,
                .bounce = 0.1f,
                .center = {0.0f, 0.75f, 0.0f},
            },
        },
    },
};

void npc_interact(struct interactable* interactable, entity_id from) {
    struct npc* npc = (struct npc*)interactable->data;
    cutscene_ref_run(&npc->talk_to_cutscene, npc->cutscene_actor.collider.entity_id);
}

void npc_update(void *data) {
    struct npc* npc = (struct npc*)data;

    cutscene_actor_update(&npc->cutscene_actor);
}

void npc_init(struct npc* npc, struct npc_definition* definiton, entity_id id) {
    struct npc_information* information = &npc_information[definiton->npc_type];

    transformSaInit(&npc->cutscene_actor.transform, &definiton->position, &definiton->rotation, 1.0f);
    renderable_single_axis_init(&npc->renderable, &npc->cutscene_actor.transform, information->mesh);

    render_scene_add_renderable(&npc->renderable, 2.0f);

    update_add(npc, npc_update, 0, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    cutscene_actor_init(
        &npc->cutscene_actor, 
        &information->actor,
        id,
        &npc->cutscene_actor.transform, 
        &npc->renderable.armature, 
        information->animations
    );

    
    if (*definiton->dialog) {
        interactable_init(&npc->interactable, id, INTERACT_TYPE_TALK, npc_interact, npc);
        cutscene_ref_init(&npc->talk_to_cutscene, definiton->dialog);
    } else {
        cutscene_ref_init_null(&npc->talk_to_cutscene);
    }
}

void npc_destroy(struct npc* npc) {
    render_scene_remove(&npc->renderable);
    renderable_destroy(&npc->renderable);
    cutscene_actor_destroy(&npc->cutscene_actor);
    update_remove(npc);
    if (npc->talk_to_cutscene.type != CUTSCENE_REF_NONE) {
        interactable_destroy(&npc->interactable);
        cutscene_ref_destroy(&npc->talk_to_cutscene);
    }
}

void npc_common_init() {

}

void npc_common_destroy() {

}