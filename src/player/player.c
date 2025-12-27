#include "player.h"

#include <libdragon.h>

#include "../collision/collision_scene.h"
#include "../collision/shapes/capsule.h"
#include "../collision/shapes/cylinder.h"
#include "../entity/interactable.h"
#include "../math/vector2.h"
#include "../render/render_scene.h"
#include "../resource/animation_cache.h"
#include "../resource/tmesh_cache.h"
#include "../time/time.h"
#include "../debug/debug_colliders.h"
#include "../render/defs.h"
#include "../audio/audio.h"
#include "../entities/vehicle.h"
#include "../menu/map_menu.h"

#include "../effects/fade_effect.h"

#define PLAYER_MAX_SPEED    4.2f

#define PLAYER_DASH_THRESHOLD 4.7f
#define PLAYER_RUN_THRESHOLD 1.4f
#define PLAYER_RUN_ANIM_SPEED   4.2f
#define PLAYER_WALK_ANIM_SPEED   0.64f

#define SLIDE_DELAY 0.25f
#define COYOTE_TIME 0.1f
#define SHADOW_AS_GROUND_DISTANCE   0.15f

#define CARRY_GRAB_TIME   (11.0f / 30.0f)
#define CARRY_DROP_TIME   (11.0f / 30.0f)

#define INTERACT_Y_LOWER        -0.75f
#define INTERACT_Y_UPEER        1.25f

#define PLAYER_LAYERS   (COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_LIGHTING_TANGIBLE | COLLISION_LAYER_DAMAGE_PLAYER)


#define MAX_ROTATION_RATE       5.0f
static struct Vector2 player_max_rotation;
static struct Vector2 z_target_rotation;

static struct spatial_trigger_type player_vision_shape = {
    SPATIAL_TRIGGER_WEDGE(15.0f, 7.0f, 0.707f, 0.707f),
};

struct climb_up_data {
    float max_climb_height;
    float animation_height;
    float start_jump_time;
    float end_jump_time;
};

typedef struct climb_up_data climb_up_data_t;

// about a 40 degree slope
#define MAX_STABLE_SLOPE    0.219131191f

#define MAX_SLIDING_SLOPE   0.8f

#define LOOK_DEADZONE       0.03f
#define MOVE_DEADZONE       0.1f

static struct cutscene_actor_def player_actor_def = {
    .eye_level = 1.26273f,
    .move_speed = PLAYER_WALK_ANIM_SPEED,
    .run_speed = PLAYER_RUN_ANIM_SPEED,
    .run_threshold = PLAYER_RUN_THRESHOLD,
    .rotate_speed = 2.0f,
    .collision_layers = PLAYER_LAYERS,
    .collision_group = COLLISION_GROUP_PLAYER,
    .collider = {
        .minkowsi_sum = capsule_minkowski_sum,
        .bounding_box = capsule_bounding_box,
        .data = {
            .capsule = {
                .radius = 0.25f,
                .inner_half_height = 0.5f,
            }
        },
        .max_stable_slope = MAX_STABLE_SLOPE,
        .friction = 0.2f,
        .center = {0.0f, 0.75f, 0.0f},
    },
};

void player_get_move_basis(struct Transform* transform, struct Vector3* forward, struct Vector3* right) {
    quatMultVector(&transform->rotation, &gForward, forward);
    quatMultVector(&transform->rotation, &gRight, right);

    if (forward->y > 0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
        vector3Negate(forward, forward);
    } else if (forward->y < -0.7f) {
        quatMultVector(&transform->rotation, &gUp, forward);
    }

    forward->y = 0.0f;
    right->y = 0.0f;

    vector3Normalize(forward, forward);
    vector3Normalize(right, right);
}

void player_run_clip(struct player* player, enum player_animation clip) {
    animator_run_clip(&player->cutscene_actor.animator, player->animations[clip], 0.0f, false);
    player->cutscene_actor.animate_speed = 1.0f;
}

void player_run_clip_keep_translation(struct player* player, enum player_animation clip) {
    struct Transform before;
    armature_bone_transform(player->cutscene_actor.armature, 0, &before);
    player_run_clip(player, clip);
    animator_update(&player->cutscene_actor.animator, player->cutscene_actor.armature, fixed_time_step);
    struct Transform after;
    armature_bone_transform(player->cutscene_actor.armature, 0, &after);

    struct Vector3 final_point;
    transformSaTransformPoint(&player->cutscene_actor.transform, &before.position, &final_point);
    vector3AddScaled(
        &player->cutscene_actor.transform.position, 
        &final_point, 
        1.0f / MODEL_SCALE, 
        &player->cutscene_actor.transform.position
    );
    
    transformSaTransformPoint(&player->cutscene_actor.transform, &after.position, &final_point);
    vector3AddScaled(
        &player->cutscene_actor.transform.position, 
        &final_point, 
        -1.0f / MODEL_SCALE,
        &player->cutscene_actor.transform.position
    );
}

bool player_is_running(struct player* player, enum player_animation clip) {
    return animator_is_running_clip(&player->cutscene_actor.animator, player->animations[clip]);
}

void player_loop_animation(struct player* player, enum player_animation clip, float speed) {
    if (!animator_is_running_clip(&player->cutscene_actor.animator, player->animations[clip])) {
        animator_run_clip(&player->cutscene_actor.animator, player->animations[clip], 0.0f, true);
    }
    player->cutscene_actor.animate_speed = speed;
}

void player_get_input_direction(struct player* player, struct Vector3* target_direction) {
    joypad_inputs_t input = joypad_get_inputs(0);

    struct Vector3 right;
    struct Vector3 forward;

    player_get_move_basis(player->camera_transform, &forward, &right);

    struct Vector2 direction;

    direction.x = input.stick_x * (1.0f / 80.0f);
    direction.y = -input.stick_y * (1.0f / 80.0f);

    float magSqrd = vector2MagSqr(&direction);

    if (magSqrd > 1.0f) {
        vector2Scale(&direction, 1.0f / sqrtf(magSqrd), &direction);
    }

    vector3Scale(&right, target_direction, direction.x);
    vector3AddScaled(target_direction, &forward, direction.y, target_direction);
}

void player_look_towards(struct player* player, struct Vector3* target_direction) {
    if (vector3MagSqrd(target_direction) > LOOK_DEADZONE) {
        struct Vector2 directionUnit;
        vector2LookDir(&directionUnit, target_direction);
        vector2RotateTowards(&player->cutscene_actor.transform.rotation, &directionUnit, &player_max_rotation, &player->cutscene_actor.transform.rotation);
    }
}

void player_handle_look(struct player* player, struct Vector3* look_direction) {
    player_look_towards(player, look_direction);
}

void player_enter_grounded_state(struct player* player) {
    player->coyote_time = 0.0f;
    player->state = PLAYER_GROUNDED;
    player_loop_animation(player, PLAYER_ANIMATION_IDLE, 1.0f);
}

void player_enter_vehicle(struct player* player, entity_id vehicle_id) {
    vehicle_t* vehicle = vehicle_get(vehicle_id);

    if (!vehicle) {
        return;
    }

    player->state_data.in_vehicle.target = vehicle_id;
    player->state = PLAYER_IN_VEHICLE;
    player->hover_interaction = 0;
    vehicle_enter(vehicle, ENTITY_ID_PLAYER);
    player->cutscene_actor.collider.collision_layers = 0;
    player_loop_animation(player, PLAYER_ANIMATION_RIDE_BIKE, 1.0f);
}

void player_exit_vehicle(struct player* player) {
    if (player->state != PLAYER_IN_VEHICLE) {
        return;
    }

    vehicle_t* vehicle = vehicle_get(player->state_data.in_vehicle.target);
    player->cutscene_actor.collider.collision_layers = PLAYER_LAYERS;

    player_enter_grounded_state(player);
    
    if (vehicle) {
        vehicle_apply_exit_transform(vehicle, &player->cutscene_actor.transform);
        vehicle_exit(vehicle);
    }
}

bool player_handle_ground_movement(struct player* player, struct contact* ground_contact, struct Vector3* target_direction, float* speed) {
    contact_t fake_contact = {
        .normal = gUp,
        .point = player->cutscene_actor.transform.position,
        .surface_type = SURFACE_TYPE_COYOTE,
    };
    
    *speed = sqrtf(vector3MagSqrd2D(&player->cutscene_actor.collider.velocity));

    if (ground_contact) {
        player->coyote_time = 0.0f;
    } else if (player->coyote_time < COYOTE_TIME) {
        player->coyote_time += fixed_time_step;
        ground_contact = &fake_contact;
    } else {
        return false;
    }

    if (dynamic_object_should_slide(MAX_STABLE_SLOPE, ground_contact->normal.y, ground_contact->surface_type)) {
        // TODO handle sliding logic
        player->slide_timer += fixed_time_step;

        if (player->slide_timer > SLIDE_DELAY) {
            return true;
        }
    } else {
        player->slide_timer = 0.0f;
    }

    player_handle_look(player, target_direction);

    if (player->cutscene_actor.collider.is_pushed) {
        return true;
    }

    float movement_alignment;

    struct Vector2 target_rotation;
    vector2LookDir(&target_rotation, target_direction);
    movement_alignment = vector2Dot(&player->cutscene_actor.transform.rotation, &target_rotation);

    if (vector3MagSqrd(target_direction) < MOVE_DEADZONE || movement_alignment < 0.0f) {
        player->cutscene_actor.collider.velocity = gZeroVec;
    } else {
        // TODO adjust for deadzone
        struct Vector3 projected_target_direction;
        vector3ProjectPlane(target_direction, &ground_contact->normal, &projected_target_direction);

        struct Vector3 normalized_direction;
        vector3Normalize(target_direction, &normalized_direction);
        struct Vector3 projected_normalized;
        vector3ProjectPlane(&normalized_direction, &ground_contact->normal, &projected_normalized);

        vector3Scale(&projected_target_direction, &projected_target_direction, movement_alignment / sqrtf(vector3MagSqrd(&projected_normalized)));

        float prev_y = player->cutscene_actor.collider.velocity.y;

        vector3Scale(&projected_target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
        if (ground_contact->surface_type == SURFACE_TYPE_COYOTE) {
            player->cutscene_actor.collider.velocity.y = prev_y;
        }
    }

    if (ground_contact->other_object) {
        struct dynamic_object* ground_object = collision_scene_find_object(ground_contact->other_object);

        if (ground_object) {
            vector3Add(&player->cutscene_actor.collider.velocity, &ground_object->velocity, &player->cutscene_actor.collider.velocity);
        }
    } else if (ground_contact->surface_type != SURFACE_TYPE_COYOTE) {
        player->last_good_footing = player->cutscene_actor.transform.position;
    }

    return true;
}

void player_handle_air_movement(struct player* player, contact_t* ground_contact) {
    if (player->cutscene_actor.collider.is_pushed) {
        return;
    }

    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    player_handle_look(player, &target_direction);

    if (ground_contact && vector3Dot(&target_direction, &ground_contact->normal) < 0.0f) {
        vector3ProjectPlane(&target_direction, &ground_contact->normal, &target_direction);
    }

    float prev_y = player->cutscene_actor.collider.velocity.y;
    vector3Scale(&target_direction, &player->cutscene_actor.collider.velocity, PLAYER_MAX_SPEED);
    player->cutscene_actor.collider.velocity.y = prev_y;
}

interactable_t* player_find_interactable(struct player* player) {
    interactable_t* result = NULL;
    float distance = player_vision_shape.data.wedge.radius * player_vision_shape.data.wedge.radius;

    vector3_t* pos = cutscene_actor_get_pos(&player->cutscene_actor);

    for (contact_t* contact = player->vision.active_contacts;
        contact;
        contact = contact->next) {
        dynamic_object_t* obj = collision_scene_find_object(contact->other_object);

        if (!obj) {
            continue;
        }

        struct Vector3 offset;
        vector3Sub(pos, obj->position, &offset);
        float distanceCheck = vector3MagSqrd2D(&offset);

        if (distanceCheck >= distance) {
            continue;
        }

        interactable_t* interactable = interactable_get(contact->other_object);

        if (!interactable || !interactable_is_in_range(interactable, distanceCheck)) {
            continue;
        }

        result = interactable;
        distance = distanceCheck;
    }

    return result;
}

void player_interact(struct player* player, interactable_t* interactable) {
    interactable->callback(interactable, ENTITY_ID_PLAYER);

    if (interactable->interact_type == INTERACT_TYPE_RIDE) {
        player_enter_vehicle(player, interactable->id);
    }
}

void player_update_grounded(struct player* player, struct contact* ground_contact) {
    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (ground_contact && dynamic_object_should_slide(MAX_SLIDING_SLOPE, ground_contact->normal.y, SURFACE_TYPE_DEFAULT)) {
        return;
    }
    
    interactable_t* interactable = player_find_interactable(player);

    if (interactable) {
        player->hover_interaction = interactable->id;
        if (pressed.a) {
            player_interact(player, interactable);
        }
    } else {
        player->hover_interaction = 0;
    }
 
    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);

    float speed;
    if (!player_handle_ground_movement(player, ground_contact, &target_direction, &speed)) {
        return;
    }
}

void player_update_in_vehicle(struct player* player, struct contact* ground_contact) {
    vehicle_t* vehicle = vehicle_get(player->state_data.in_vehicle.target);

    if (!vehicle) {
        player_exit_vehicle(player);
        return;
    }

    struct Vector3 target_direction;
    player_get_input_direction(player, &target_direction);
    vehicle_steer(vehicle, &target_direction);
    vehicle_apply_driver_transform(vehicle, &player->cutscene_actor.transform);
    player->cutscene_actor.collider.velocity = gZeroVec;

    joypad_buttons_t pressed = joypad_get_buttons_pressed(0);

    if (pressed.b && vehicle->is_stopped) {
        player_exit_vehicle(player);
    }
}

void player_update_state(struct player* player, struct contact* ground_contact) {
    switch (player->state) {
         case PLAYER_GROUNDED:
            player_update_grounded(player, ground_contact);
            break;
        case PLAYER_IN_VEHICLE:
            player_update_in_vehicle(player, ground_contact);
            break;
        default:
            break;
    }
}

void player_update(struct player* player) {
    if (cutscene_actor_update(&player->cutscene_actor) || !update_has_layer(UPDATE_LAYER_WORLD)) {
        return;
    }

    map_mark_revealed(player_get_position(player));

    struct contact* ground = dynamic_object_get_ground(&player->cutscene_actor.collider);
    
    player_update_state(player, ground);

    if (player->cutscene_actor.collider.hit_kill_plane) {
        player->cutscene_actor.transform.position = player->last_good_footing;
        player->cutscene_actor.collider.velocity = gZeroVec;
        player->cutscene_actor.collider.hit_kill_plane = 0;
    }
}

float player_on_damage(void* data, struct damage_info* damage) {
    return damage->amount;
}

static const char* animation_clip_names[PLAYER_ANIMATION_COUNT] = {
    [PLAYER_ANIMATION_IDLE] = "idle",
    [PLAYER_ANIMATION_RIDE_BIKE] = "ride_bike",
};

static const char* sound_names[PLAYER_SOUND_COUNT] = {};

void player_load_animation(struct player* player) {
    for (int i = 0; i < PLAYER_ANIMATION_COUNT; i += 1) {
        player->animations[i] = animation_set_find_clip(player->cutscene_actor.animation_set, animation_clip_names[i]);
    }
}

void player_load_sound(struct player* player) {
    for (int i = 0; i < PLAYER_SOUND_COUNT; i += 1) {
        player->sounds[i] = wav64_load(sound_names[i], NULL);
    }
}

void player_unload_sound(struct player* player) {
    for (int i = 0; i < PLAYER_SOUND_COUNT; i += 1) {
        wav64_close(player->sounds[i]);
    }
}

void player_init(struct player* player, struct player_definition* definition, struct Transform* camera_transform) {
    transformSaInitIdentity(&player->cutscene_actor.transform);
    renderable_single_axis_init(&player->renderable, &player->cutscene_actor.transform, "rom:/meshes/characters/scrapper_kid.tmesh");

    player->camera_transform = camera_transform;

    player->cutscene_actor.transform.position = definition->location;
    player->cutscene_actor.transform.rotation = definition->rotation;

    player->last_good_footing = definition->location;
    player->coyote_time = 0.0f;

    render_scene_add_renderable(&player->renderable, 2.0f);
    update_add(player, (update_callback)player_update, UPDATE_PRIORITY_PLAYER, UPDATE_LAYER_WORLD | UPDATE_LAYER_CUTSCENE);

    vector2ComplexFromAngle(fixed_time_step * MAX_ROTATION_RATE, &player_max_rotation);
    vector2ComplexFromAngle(fixed_time_step * 2.0f, &z_target_rotation);

    struct TransformSingleAxis transform = {
        .position = definition->location,
        .rotation = definition->rotation,
        .scale = 1.0f,
    };

    cutscene_actor_init(
        &player->cutscene_actor,
        &player_actor_def,
        ENTITY_ID_PLAYER,
        &transform,
        &player->renderable.armature,
        "rom:/meshes/characters/scrapper_kid.anim"
    );

    player->cutscene_actor.collider.density_class = DYNAMIC_DENSITY_MEDIUM;
    player->cutscene_actor.collider.weight_class = WEIGHT_CLASS_MEDIUM;

    health_init(&player->health, ENTITY_ID_PLAYER, 100.0f);
    health_set_callback(&player->health, player_on_damage, player);

    player_load_animation(player);
    player_load_sound(player);

    player->last_spell_animation = NULL;

    player->state = PLAYER_GROUNDED;

    spatial_trigger_init(
        &player->vision,
        &player->cutscene_actor.transform, 
        &player_vision_shape, 
        COLLISION_LAYER_TANGIBLE | COLLISION_LAYER_INTERACT_ONLY, 
        ENTITY_ID_PLAYER
    );
    collision_scene_add_trigger(&player->vision);
    player_loop_animation(player, PLAYER_ANIMATION_IDLE, 1.0f);
}

void player_destroy(struct player* player) {
    player_exit_vehicle(player);

    renderable_destroy(&player->renderable);
    health_destroy(&player->health);

    render_scene_remove(&player->renderable);
    update_remove(player);
    cutscene_actor_destroy(&player->cutscene_actor);
    collision_scene_remove_trigger(&player->vision);

    player_unload_sound(player);
}