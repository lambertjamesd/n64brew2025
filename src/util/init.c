#include "init.h"

#include "../render/render_scene.h"
#include "../time/time.h"
#include "../collision/collision_scene.h"
#include "../entity/health.h"
#include "../entity/interactable.h"
#include "../menu/menu_rendering.h"
#include "../menu/dialog_box.h"
#include "../cutscene/cutscene_runner.h"
#include "../cutscene/cutscene_actor.h"
#include "../audio/audio.h"
#include "../entities/vehicle.h"

void init_engine() {
    render_scene_reset();
    update_reset();
    collision_scene_reset();
    health_reset();
    vehicle_global_reset();
    menu_reset();
    cutscene_runner_init();
    cutscene_actor_reset();
    audio_player_init();
}