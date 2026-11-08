#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "2s2h/Enhancements/EnemyOverhaul/EnemyOverhaul.h"
}

#define ENCOUNTERS_OR_ELITES (EO_ENCOUNTERS_ON() || EO_ELITES_ON())

void RegisterEnemyOverhaul() {
    // Shots and summons: always kept tidy, whatever the toggles, so nothing is left pointing at a freed actor.
    COND_HOOK(OnActorDestroy, true, [](Actor* actor) { EnemyOverhaul_OnActorDestroy(actor); });

    // Enemies and projectiles borrowed from other areas don't need their object in the room's object list: assets are
    // loaded by name. Only affects actors spawned through Eo_SpawnAnyObject.
    COND_VB_SHOULD(VB_ENABLE_OBJECT_DEPENDENCY, true, {
        if (gEoSpawningAnyObject > 0) {
            *should = false;
        }
    });

    // Companions, tag-alongs, elites and the Final Day frenzy
    COND_HOOK(OnActorInit, ENCOUNTERS_OR_ELITES, [](Actor* actor) { EnemyOverhaul_OnActorInit(actor); });
    COND_HOOK(OnActorUpdate, ENCOUNTERS_OR_ELITES, [](Actor* actor) { EnemyOverhaul_OnActorUpdate(actor); });
    COND_HOOK(OnActorKill, EO_ELITES_ON(), [](Actor* actor) { EnemyOverhaul_OnActorKill(actor); });
    COND_HOOK(ShouldActorDraw, EO_ELITES_ON(),
              [](Actor* actor, bool* should) { EnemyOverhaul_OnActorPreDraw(actor); });
}

static RegisterShipInitFunc initFunc(RegisterEnemyOverhaul,
                                     { EO_CVAR_MINIBOSS_HP, EO_CVAR_MINIBOSS_ATTACKS, EO_CVAR_ENEMIES, EO_CVAR_EYEGORE,
                                       EO_CVAR_DEEP_PYTHON, EO_CVAR_DEEP_PYTHON_FREE, EO_CVAR_THEM,
                                       EO_CVAR_ENCOUNTERS, EO_CVAR_ENCOUNTERS_DUNGEONS, EO_CVAR_ELITES,
                                       EO_CVAR_FRENZY });
