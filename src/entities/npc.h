#ifndef __NPC_NPC_H__
#define __NPC_NPC_H__

#include "../scene/scene_definition.h"
#include "../math/transform_single_axis.h"
#include "../render/renderable.h"
#include "../render/animator.h"
#include "../entity/interactable.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_actor.h"
#include "../cutscene/cutscene_reference.h"

struct npc_information {
    char* mesh;
    char* animations;
    struct dynamic_object_type collider;
    float half_height;
    struct cutscene_actor_def actor;
};

struct npc {
    struct cutscene_actor cutscene_actor;
    struct renderable renderable;
    struct interactable interactable;
    cutscene_ref_t talk_to_cutscene;
};

void npc_init(struct npc* npc, struct npc_definition* definiton, entity_id id);
void npc_destroy(struct npc* npc);
void npc_common_init();
void npc_common_destroy();

#endif