// 2S2H-MM-Overhaul: Giant's Mask and Fierce Deity's Mask always use magic
//
// The Giant's Mask drains magic while worn and the Fierce Deity's Mask is made to drain it the same way, at the
// Giant's Mask's rate. Neither the Infinite Magic cheat nor Chateau Romani stops the drain. When the magic runs out,
// Link turns back: the game already does this for the Giant's Mask, and Player does it for the Fierce Deity
// (see MasksAlwaysUseMagic_OutOfMagic in z_player.c).

#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"
#include "MasksAlwaysUseMagic.h"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

#define CVAR_NAME "gEnhancements.Masks.AlwaysUseMagic"

static s16 sDrainTimer = 0;

static bool MasksAlwaysUseMagic_IsGiant(Player* player) {
    return (player->transformation == PLAYER_FORM_HUMAN) && (player->currentMask == PLAYER_MASK_GIANT);
}

static bool MasksAlwaysUseMagic_IsFierceDeity(Player* player) {
    return player->transformation == PLAYER_FORM_FIERCE_DEITY;
}

extern "C" bool MasksAlwaysUseMagic_Active(void) {
    if (!CVarGetInteger(CVAR_NAME, 1) || (gPlayState == NULL)) {
        return false;
    }
    Player* player = GET_PLAYER(gPlayState);

    return (player != NULL) && (MasksAlwaysUseMagic_IsGiant(player) || MasksAlwaysUseMagic_IsFierceDeity(player));
}

extern "C" bool MasksAlwaysUseMagic_FierceDeityOutOfMagic(Player* player) {
    return CVarGetInteger(CVAR_NAME, 1) && MasksAlwaysUseMagic_IsFierceDeity(player) &&
           (gSaveContext.save.saveInfo.playerData.magic == 0);
}

static void MasksAlwaysUseMagic_Update() {
    PlayState* play = gPlayState;

    if ((play == NULL) || (gSaveContext.gameMode != GAMEMODE_NORMAL)) {
        return;
    }
    Player* player = GET_PLAYER(play);

    if ((player == NULL) || !(MasksAlwaysUseMagic_IsGiant(player) || MasksAlwaysUseMagic_IsFierceDeity(player))) {
        sDrainTimer = 0;
        return;
    }

    // The game already drains the Giant's Mask, except after Chateau Romani
    if (MasksAlwaysUseMagic_IsGiant(player) && (gSaveContext.magicState == MAGIC_STATE_CONSUME_GIANTS_MASK) &&
        !CHECK_WEEKEVENTREG(WEEKEVENTREG_DRANK_CHATEAU_ROMANI)) {
        sDrainTimer = 0;
        return;
    }

    // Same pauses as the game's own drain
    if (IS_PAUSED(&play->pauseCtx) || (play->msgCtx.msgMode != MSGMODE_NONE) ||
        (play->gameOverCtx.state != GAMEOVER_INACTIVE) || (play->transitionTrigger != TRANS_TRIGGER_OFF) ||
        (play->transitionMode != TRANS_MODE_OFF) || Play_InCsMode(play)) {
        return;
    }

    if (sDrainTimer <= 0) {
        sDrainTimer = (R_MAGIC_CONSUME_TIMER_GIANTS_MASK > 0) ? R_MAGIC_CONSUME_TIMER_GIANTS_MASK : 80;
    }
    sDrainTimer--;
    if (sDrainTimer == 0) {
        if (gSaveContext.save.saveInfo.playerData.magic > 0) {
            gSaveContext.save.saveInfo.playerData.magic--;
        }
    }
}

static void RegisterMasksAlwaysUseMagic() {
    COND_HOOK(OnGameStateUpdate, CVarGetInteger(CVAR_NAME, 1), MasksAlwaysUseMagic_Update);
}

static RegisterShipInitFunc initFunc(RegisterMasksAlwaysUseMagic, { CVAR_NAME });
