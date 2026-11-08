// 2S2H-MM-Overhaul: Secret Detection
//
// Shows clues near undiscovered secrets using sparkle and dust effects already in the game; no new assets.
// Every secret here is its own actor; a broken wall or container is gone, and a collected Stray Fairy or opened
// chest sets a flag, so a clue only shows while its secret is still waiting to be found.
//
// The clue's colour says what to do:
//   gold + dust - explosives (bombable walls and floors, breakable boulders, bomb-only grottos)
//   purple      - the Lens of Truth (Lens platforms, objects only the Lens shows, invisible chests)
//   pink        - a hidden Stray Fairy (in a pot, beehive, barrel, crate, flower pot, snowball or chest)
//   white       - invisible Rupees

#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

extern "C" {
#include "macros.h"
#include "variables.h"
#include "functions.h"
extern PlayState* gPlayState;
}

#define SD_CVAR_ENABLED "gEnhancements.SecretDetection.Enabled"
#define SD_CVAR_EXPLOSIVE "gEnhancements.SecretDetection.Explosive"
#define SD_CVAR_LENS "gEnhancements.SecretDetection.Lens"
#define SD_CVAR_FAIRIES "gEnhancements.SecretDetection.StrayFairies"
#define SD_CVAR_RUPEES "gEnhancements.SecretDetection.Rupees"
#define SD_CVAR_STRENGTH "gEnhancements.SecretDetection.Strength" // 0 = Subtle, 1 = Obvious
#define SD_CVAR_NEED_LENS "gEnhancements.SecretDetection.RequireLens"

typedef enum {
    SECRET_EXPLOSIVE,
    SECRET_LENS,
    SECRET_FAIRY,
    SECRET_RUPEE,
} SecretKind;

static bool SecretDetection_On() {
    return CVarGetInteger(SD_CVAR_ENABLED, 1) != 0;
}

static bool SecretDetection_KindOn(const char* cvar) {
    return SecretDetection_On() && CVarGetInteger(cvar, 1) != 0;
}

static bool SecretDetection_LensActive() {
    return (gPlayState != NULL) && gPlayState->actorCtx.lensActive;
}

// Spawns one pulse of clues at a secret, if Link is close enough and it's this secret's turn.
static void SecretDetection_Pulse(Actor* actor, SecretKind kind) {
    PlayState* play = gPlayState;
    const bool obvious = CVarGetInteger(SD_CVAR_STRENGTH, 1) != 0;
    const f32 range = obvious ? 500.0f : 250.0f;
    const u32 period = obvious ? 20 : 40;
    const s32 sparkles = obvious ? 3 : 2;

    if (play == NULL) {
        return;
    }
    if (CVarGetInteger(SD_CVAR_NEED_LENS, 0) && (INV_CONTENT(ITEM_LENS_OF_TRUTH) != ITEM_LENS_OF_TRUTH)) {
        return;
    }
    if (actor->xyzDistToPlayerSq > range * range) {
        return;
    }
    // Spread secrets across frames so several nearby ones don't all pulse together
    if (((play->gameplayFrames + (u32)((uintptr_t)actor >> 4)) % period) != 0) {
        return;
    }

    // Sparkle colours per kind, in SecretKind order: gold, purple, pink, white
    static const Color_RGBA8 kPrim[] = {
        { 255, 230, 120, 255 }, { 230, 170, 255, 255 }, { 255, 190, 230, 255 }, { 255, 255, 255, 255 }
    };
    static const Color_RGBA8 kEnv[] = { { 255, 140, 0, 0 }, { 140, 0, 200, 0 }, { 255, 80, 170, 0 }, { 200, 200, 200, 0 } };
    Color_RGBA8 prim = kPrim[kind];
    Color_RGBA8 env = kEnv[kind];

    Vec3f vel = { 0.0f, 0.4f, 0.0f };
    Vec3f accel = { 0.0f, 0.02f, 0.0f };

    for (s32 i = 0; i < sparkles; i++) {
        Vec3f pos = actor->world.pos;
        pos.x += Rand_CenteredFloat(70.0f);
        pos.z += Rand_CenteredFloat(70.0f);
        pos.y += 10.0f + Rand_ZeroOne() * 60.0f;
        EffectSsKirakira_SpawnDispersed(play, &pos, &vel, &accel, &prim, &env, 1200, 24);
    }

    // Explosive secrets also puff a little dust, like a crack about to give way
    if ((kind == SECRET_EXPLOSIVE) && obvious) {
        Color_RGBA8 dust = { 170, 150, 120, 255 };
        Vec3f dustVel = { Rand_CenteredFloat(1.0f), 0.8f, Rand_CenteredFloat(1.0f) };
        Vec3f dustAccel = { 0.0f, 0.05f, 0.0f };
        Vec3f pos = actor->world.pos;

        pos.x += Rand_CenteredFloat(50.0f);
        pos.z += Rand_CenteredFloat(50.0f);
        pos.y += 5.0f + Rand_ZeroOne() * 30.0f;
        EffectSsDust_Spawn(play, 0, &pos, &dustVel, &dustAccel, &dust, &dust, 50, 5, 10, 0);
    }
}

// ---- Clue handlers --------------------------------------------------------------------------------

static void SecretDetection_OnExplosive(Actor* actor) {
    SecretDetection_Pulse(actor, SECRET_EXPLOSIVE);
}

// Grottos that open with an explosion keep DOORANA_TYPE_HIDDEN_BOMB (0x200) in their params until opened.
// (Song of Storms grottos are an unfinished leftover from Ocarina of Time in this game.)
static void SecretDetection_OnGrotto(Actor* actor) {
    if ((actor->params & 0x300) == 0x200) {
        SecretDetection_Pulse(actor, SECRET_EXPLOSIVE);
    }
}

// Lens secrets only need a clue while the Lens of Truth isn't already showing them
static void SecretDetection_OnLens(Actor* actor) {
    if (!SecretDetection_LensActive()) {
        SecretDetection_Pulse(actor, SECRET_LENS);
    }
}

static void SecretDetection_OnRupee(Actor* actor) {
    SecretDetection_Pulse(actor, SECRET_RUPEE);
}

// Containers: the game's own check for "this still holds an uncollected Stray Fairy". All of these keep the drop
// index in params & 0x3F; the collectible flag is at bit 8 for most and at bit 9 for pots.
static void SecretDetection_OnContainer(Actor* actor) {
    if ((gPlayState != NULL) && Item_CanDropBigFairy(gPlayState, actor->params & 0x3F, (actor->params >> 8) & 0x7F)) {
        SecretDetection_Pulse(actor, SECRET_FAIRY);
    }
}

static void SecretDetection_OnPot(Actor* actor) {
    if ((gPlayState != NULL) && Item_CanDropBigFairy(gPlayState, actor->params & 0x3F, (actor->params >> 9) & 0x7F)) {
        SecretDetection_Pulse(actor, SECRET_FAIRY);
    }
}

// Big crates only drop collectibles when their contents bit (0x8000) is clear; otherwise they hide a Skulltula.
static void SecretDetection_OnBigCrate(Actor* actor) {
    if (((actor->params >> 0xF) & 1) == 0) {
        SecretDetection_OnContainer(actor);
    }
}

// Chests: a Stray Fairy chest until it's opened (pink), and an invisible chest until the Lens shows it (purple).
// The item is in params bits 5-11 and the chest flag in bits 0-4.
static void SecretDetection_OnChest(Actor* actor) {
    PlayState* play = gPlayState;

    if (play == NULL) {
        return;
    }
    if (CVarGetInteger(SD_CVAR_FAIRIES, 1) && (((actor->params >> 5) & 0x7F) == GI_STRAY_FAIRY) &&
        !Flags_GetTreasure(play, actor->params & 0x1F)) {
        SecretDetection_Pulse(actor, SECRET_FAIRY);
    } else if (CVarGetInteger(SD_CVAR_LENS, 1) && (actor->flags & ACTOR_FLAG_REACT_TO_LENS) &&
               !SecretDetection_LensActive()) {
        SecretDetection_Pulse(actor, SECRET_LENS);
    }
}

// ---- Registration ---------------------------------------------------------------------------------

static void RegisterSecretDetection() {
    const bool explosive = SecretDetection_KindOn(SD_CVAR_EXPLOSIVE);
    const bool lens = SecretDetection_KindOn(SD_CVAR_LENS);
    const bool fairies = SecretDetection_KindOn(SD_CVAR_FAIRIES);
    const bool rupees = SecretDetection_KindOn(SD_CVAR_RUPEES);
    const bool chests = fairies || lens;

    // Bombable walls and floors, breakable boulders, bomb-only grottos
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_ASTR_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HAKA_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_HAKUGIN_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_IKANA_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_KIN2_BOMBWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_BG_LAST_BWALL, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_BOMBIWA, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_HAMISHI, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_HUGEBOMBIWA, explosive, SecretDetection_OnExplosive);
    COND_ID_HOOK(OnActorUpdate, ACTOR_DOOR_ANA, explosive, SecretDetection_OnGrotto);

    // Lens of Truth platforms and objects only the Lens shows
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_VISIBLOCK, lens, SecretDetection_OnLens);
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_TEST2, lens, SecretDetection_OnLens);

    // Hidden Stray Fairies
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_TSUBO, fairies, SecretDetection_OnPot);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_COMB, fairies, SecretDetection_OnContainer);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_TARU, fairies, SecretDetection_OnContainer);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_KIBAKO, fairies, SecretDetection_OnContainer);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_KIBAKO2, fairies, SecretDetection_OnBigCrate);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_FLOWERPOT, fairies, SecretDetection_OnContainer);
    COND_ID_HOOK(OnActorUpdate, ACTOR_OBJ_SNOWBALL, fairies, SecretDetection_OnContainer);

    // Chests: Stray Fairy chests and invisible chests
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_BOX, chests, SecretDetection_OnChest);

    // Invisible Rupees
    COND_ID_HOOK(OnActorUpdate, ACTOR_EN_INVISIBLE_RUPPE, rupees, SecretDetection_OnRupee);
}

static RegisterShipInitFunc initFunc(RegisterSecretDetection,
                                     { SD_CVAR_ENABLED, SD_CVAR_EXPLOSIVE, SD_CVAR_LENS, SD_CVAR_FAIRIES, SD_CVAR_RUPEES });
