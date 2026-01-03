#include "boost_pad.h"

#include "../render/tmesh.h"
#include "../render/render_scene.h"
#include "../collision/shapes/box.h"
#include "../cutscene/expression_evaluate.h"
#include "../collision/collision_scene.h"
#include "../cutscene/cutscene.h"
#include "../cutscene/cutscene_runner.h"
#include "../scene/scene.h"
#include "../time/time.h"
#include "vehicle.h"

static dynamic_object_type_t interact = {
    BOX_COLLIDER(2.0f, 0.5f, 2.0f),
    .center = {0.0f, 0.5f, 0.0f},
};

static spatial_trigger_type_t boost_trigger = {
    .type = SPATIAL_TRIGGER_CYLINDER,
    .center = {0.0f, 1.0f, 0.0f},
    .data = {
        .cylinder = {
            .half_height = 1.5f,
            .radius = 2.5f,
        },
    },
};

struct boost_pad_assets {
    tmesh_t pad;
    tmesh_t outline;
};

static struct boost_pad_assets assets;
static color_t on_color = {0x00, 0xFF, 0xFB, 0x2F};
static color_t off_color = {0x03, 0x06, 0x2F, 0x2F};

void boost_pad_render(void* data, struct render_batch* batch) {
    boost_pad_t* boost_pad = (boost_pad_t*)data;

    T3DMat4FP* mtx = render_batch_transformfp_from_full(batch, &boost_pad->transform);

    if (!mtx) {
        return;
    }

    element_attr_t attrs[2];
    attrs[0] = element_attr_prim_color(boost_pad->is_on ? on_color : off_color);
    attrs[1] = element_attr_end();

    render_batch_add_tmesh(batch, &assets.pad, mtx, NULL, NULL, attrs);

    if (current_scene->player.state != PLAYER_IN_VEHICLE || !boost_pad->is_on) {
        return;
    }

    // only render when on motorcycle
    render_batch_add_tmesh(batch, &assets.outline, mtx, NULL, NULL, NULL);
}

void boost_pad_update(void* data) {
    boost_pad_t* boost_pad = (boost_pad_t*)data;

    for (
        contact_t* contact = boost_pad->boost_trigger.active_contacts;
        contact;
        contact = contact->next
    ) {
        vehicle_t* vehicle = vehicle_get(contact->other_object);

        if (vehicle) {
            vehicle->hit_boost_pad = true;
        }
    }
}

void boost_pad_interact(struct interactable* interactable, entity_id from) {
    boost_pad_t* boost_pad = (boost_pad_t*)interactable->data;
    cutscene_builder_t cutscene;
    cutscene_builder_init(&cutscene);

    cutscene_builder_pause(&cutscene, true, false, UPDATE_LAYER_WORLD);
    cutscene_builder_camera_look_at(&cutscene, boost_pad->collider.entity_id);
    cutscene_builder_dialog(&cutscene, "You need to find the base station and repair it\n");
    cutscene_builder_camera_return(&cutscene);
    cutscene_builder_pause(&cutscene, false, false, UPDATE_LAYER_WORLD);

    cutscene_runner_run(cutscene_builder_finish(&cutscene), 0, cutscene_runner_free_on_finish(), NULL, 0);
}
    
void boost_pad_init(boost_pad_t* boost_pad, struct boost_pad_definition* definition, entity_id entity_id) {
    transformInit(&boost_pad->transform, &definition->position, &definition->rotation, &gOneVec);
    transformSaInit(&boost_pad->transform_sa, &definition->position, &gRight2, 1.0f);

    render_scene_add(&boost_pad->transform.position, 2.5f, boost_pad_render, boost_pad);

    boost_pad->is_on = true; // expression_get_bool(definition->is_on);

    if (boost_pad->is_on) {
        spatial_trigger_init(&boost_pad->boost_trigger, &boost_pad->transform_sa, &boost_trigger, COLLISION_LAYER_TANGIBLE, entity_id);
        collision_scene_add_trigger(&boost_pad->boost_trigger);
        update_add(boost_pad, boost_pad_update, UPDATE_PRIORITY_PHYICS, UPDATE_LAYER_WORLD);
    } else {
        dynamic_object_init(entity_id, &boost_pad->collider, &interact, COLLISION_LAYER_INTERACT_ONLY, &boost_pad->transform.position, NULL);
        boost_pad->collider.is_fixed = true;
        boost_pad->collider.weight_class = WEIGHT_CLASS_SUPER_HEAVY;
        collision_scene_add(&boost_pad->collider);
        interactable_init(&boost_pad->interactable, entity_id, INTERACT_TYPE_CHECK, boost_pad_interact, boost_pad);
    }
}

void boost_pad_destroy(boost_pad_t* boost_pad, struct boost_pad_definition* definition) {
    render_scene_remove(boost_pad);

    if (boost_pad->is_on) {
        collision_scene_remove_trigger(&boost_pad->boost_trigger);
        update_remove(boost_pad);
    } else {
        collision_scene_remove(&boost_pad->collider);
        interactable_destroy(&boost_pad->interactable);
    }
}

void boost_pad_common_init() {
    tmesh_load_filename(&assets.pad, "rom:/meshes/vehicles/boost_pad_base.tmesh");
    tmesh_load_filename(&assets.outline, "rom:/meshes/vehicles/boost_pad_outline.tmesh");
}

void boost_pad_common_destroy() {
    tmesh_release(&assets.pad);
    tmesh_release(&assets.outline);
}
