#ifndef ENEMY_OVERHAUL_H
#define ENEMY_OVERHAUL_H

/**
 * Miniboss & Enemy Overhaul (Enhancements > Difficulty Options > Miniboss & Enemy Overhaul)
 *
 * Builds on the Boss Overhaul patch and follows the same rules:
 *  - nothing new is loaded: every attack is made of vanilla actors, effects, sounds and display lists, including other
 *    enemies' models (2S2H loads assets by name, so an object does not have to be in the room's object list);
 *  - vanilla pauses, recovery times and weak point windows are never shortened;
 *  - every vanilla mask / item trick keeps working, and quest counters (the eight Deep Pythons, the Poe Sisters' timer,
 *    Keeta's chase, the ranch invasion) keep their rules;
 *  - anything an enemy summons dies with it, so a "defeat all enemies" door can't be kept shut.
 *
 * Every toggle is on by default and is read while the game runs.
 */

#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "2s2h/Enhancements/BossOverhaul/BossOverhaul.h"

#define EO_CVAR_MINIBOSS_HP "gEnhancements.EnemyOverhaul.MinibossHP"
#define EO_CVAR_MINIBOSS_ATTACKS "gEnhancements.EnemyOverhaul.MinibossAttacks"
#define EO_CVAR_ENEMIES "gEnhancements.EnemyOverhaul.Enemies"
#define EO_CVAR_EYEGORE "gEnhancements.EnemyOverhaul.Eyegore"
#define EO_CVAR_DEEP_PYTHON "gEnhancements.EnemyOverhaul.DeepPython"
#define EO_CVAR_DEEP_PYTHON_FREE "gEnhancements.EnemyOverhaul.DeepPythonFreeSwim"
#define EO_CVAR_THEM "gEnhancements.EnemyOverhaul.Them"
#define EO_CVAR_ENCOUNTERS "gEnhancements.EnemyOverhaul.Encounters"
#define EO_CVAR_ENCOUNTERS_DUNGEONS "gEnhancements.EnemyOverhaul.EncountersInDungeons"
#define EO_CVAR_ELITES "gEnhancements.EnemyOverhaul.Elites"
#define EO_CVAR_FRENZY "gEnhancements.EnemyOverhaul.FinalDayFrenzy"

#define EO_MB_HP_ON() (CVarGetInteger(EO_CVAR_MINIBOSS_HP, 1) != 0)
#define EO_MB_ATTACKS_ON() (CVarGetInteger(EO_CVAR_MINIBOSS_ATTACKS, 1) != 0)
#define EO_ENEMIES_ON() (CVarGetInteger(EO_CVAR_ENEMIES, 1) != 0)
#define EO_EYEGORE_ON() (CVarGetInteger(EO_CVAR_EYEGORE, 1) != 0)
#define EO_PYTHON_ON() (CVarGetInteger(EO_CVAR_DEEP_PYTHON, 1) != 0)
#define EO_PYTHON_FREE_ON() (EO_PYTHON_ON() && (CVarGetInteger(EO_CVAR_DEEP_PYTHON_FREE, 1) != 0))
#define EO_THEM_ON() (CVarGetInteger(EO_CVAR_THEM, 1) != 0)
#define EO_ENCOUNTERS_ON() (CVarGetInteger(EO_CVAR_ENCOUNTERS, 1) != 0)
#define EO_ENCOUNTERS_DUNGEONS_ON() (CVarGetInteger(EO_CVAR_ENCOUNTERS_DUNGEONS, 1) != 0)
#define EO_ELITES_ON() (CVarGetInteger(EO_CVAR_ELITES, 1) != 0)
#define EO_FRENZY_ON() (CVarGetInteger(EO_CVAR_FRENZY, 1) != 0)

// Scales a miniboss' vanilla health constant, or a threshold compared against health: 2x.
#define EO_HP2(vanilla) (EO_MB_HP_ON() ? ((vanilla) * 2) : (vanilla))
// Eyegore has its own toggle.
#define EO_EYEGORE_HP(vanilla) (EO_EYEGORE_ON() ? ((vanilla) * 2) : (vanilla))

// True when `cur` is at or below num/den of `max`.
#define EO_AT_OR_BELOW(cur, max, num, den) (((s32)(cur) * (den)) <= ((s32)(max) * (num)))

/* ---------------------------------------------------------------------------------------------------------------
 * Shots: projectiles, ground waves, shockwave rings and thrown weapons that any enemy can use.
 *
 * A shot is an En_Item00 (loaded in every scene, never counted as an enemy) whose update, draw and destroy are
 * replaced; its state lives in a small pool. Orbs reuse the Boss Overhaul's orb code and looks.
 * --------------------------------------------------------------------------------------------------------------- */

typedef enum EoShotMode {
    /* 0 */ EO_SHOT_ORB,         // a Boss Overhaul orb (fire, water, electric, ember, ice, curse, light, mud)
    /* 1 */ EO_SHOT_GROUND_WAVE, // an invisible hitbox racing along the floor, trailing dust / flames / snow / water
    /* 2 */ EO_SHOT_RING,        // an expanding ring on the floor: hurts Link where the ring passes, unless he is airborne
    /* 3 */ EO_SHOT_BOOMERANG,   // a spinning display list that flies out and comes back to its owner
    /* 4 */ EO_SHOT_FALLING,     // a display list (rock etc.) dropping from above onto a marked spot
} EoShotMode;

typedef enum EoLook {
    /* 0 */ EO_LOOK_DUST,
    /* 1 */ EO_LOOK_FIRE,
    /* 2 */ EO_LOOK_SNOW,
    /* 3 */ EO_LOOK_WATER,
    /* 4 */ EO_LOOK_DARK,
} EoLook;

typedef struct EoShot {
    Actor* actor; // the host, NULL when this slot is free
    Actor* owner; // cleared when the owner is destroyed
    BoOrb orb;    // orb state and the hitbox used by every mode
    u8 mode;
    u8 look;
    u8 damage;    // EO_SHOT_GROUND_WAVE / EO_SHOT_RING / EO_SHOT_FALLING damage (0x10 is one heart)
    s16 timer;
    s16 homing;   // EO_SHOT_ORB: most it turns towards Link each frame (binang), 0 for none
    f32 speed;
    f32 radius;   // EO_SHOT_RING: current radius
    f32 radiusMax;
    f32 radiusStep;
    Vec3f target; // EO_SHOT_FALLING: landing spot
    Gfx* dList;   // EO_SHOT_BOOMERANG / EO_SHOT_FALLING model
    f32 dListScale;
    s16 spin;
    s16 spinStep;
} EoShot;

EoShot* EoShot_Get(Actor* host);
// Number of live shots of `mode` fired by `owner`.
s32 EoShot_CountOwned(Actor* owner, s32 mode);

// An orb flying with `vel`. Returns the shot, or NULL.
EoShot* EoShot_Orb(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, Vec3f* vel);
// An orb aimed at Link's chest, turned sideways by `yawOffset`.
EoShot* EoShot_OrbAimed(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s16 yawOffset);
// A fan of `count` orbs aimed at Link, `spread` apart.
void EoShot_OrbFan(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s32 count, s16 spread);
// A flat ring of `count` orbs flying outwards.
void EoShot_OrbRing(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s32 count, s16 baseYaw);
// An orb lobbed onto `to`, landing after `frames` frames.
EoShot* EoShot_OrbLob(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, Vec3f* to, f32 frames);

// A wave racing along the floor from `pos` in direction `yaw`.
EoShot* EoShot_GroundWave(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, s16 yaw, f32 speed, s16 life,
                          u8 damage);
// A fan of ground waves aimed at Link.
void EoShot_GroundWaveFan(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, s32 count, s16 spread, f32 speed,
                          s16 life, u8 damage);
// An expanding ring on the floor around `pos`. Link clears it with any jump, sidehop or backflip.
EoShot* EoShot_Ring(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, f32 radiusMax, f32 radiusStep, u8 damage);
// A spinning weapon that flies out along `yaw` and comes back to `owner`.
EoShot* EoShot_Boomerang(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* pos, s16 yaw, f32 speed,
                         u8 damage);
// Something falling from `height` above `spot`, with a shadow-like dust mark on the floor during the fall.
EoShot* EoShot_Falling(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* spot, f32 height, u8 damage);
// The same with an orb of `kind` instead of a model.
EoShot* EoShot_FallingOrb(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* spot, f32 height, u8 damage);

/* ---------------------------------------------------------------------------------------------------------------
 * Summons: vanilla enemies called in by another enemy. They die (in a puff of smoke) with their summoner.
 * --------------------------------------------------------------------------------------------------------------- */

Actor* EoSummon_Spawn(PlayState* play, Actor* owner, s16 actorId, Vec3f* pos, s16 rotY, s32 params);
s32 EoSummon_Count(Actor* owner);
void EoSummon_KillAll(Actor* owner, PlayState* play);

// Spawns any actor even if its object is not in the room's object list (see the header comment).
Actor* Eo_SpawnAnyObject(PlayState* play, s16 actorId, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz, s32 params);
// Set while Eo_SpawnAnyObject spawns something; the object dependency hook checks it.
extern s32 gEoSpawningAnyObject;

// Called by the C++ side when any actor is destroyed.
void EnemyOverhaul_OnActorDestroy(Actor* actor);

/* ---------------------------------------------------------------------------------------------------------------
 * Encounters (EoEncounters.c): companions, spawner tag-alongs, elites and the Final Day frenzy
 * --------------------------------------------------------------------------------------------------------------- */

void EnemyOverhaul_OnActorInit(Actor* actor);
void EnemyOverhaul_OnActorUpdate(Actor* actor);
void EnemyOverhaul_OnActorKill(Actor* actor);
// Called right before an actor is drawn, with its model matrix on top of the stack.
void EnemyOverhaul_OnActorPreDraw(Actor* actor);
void EoEncounters_OnActorDestroy(Actor* actor);
// True while the actor was rolled as an elite.
s32 EoElite_Is(Actor* actor);

/* ---------------------------------------------------------------------------------------------------------------
 * Small helpers
 * --------------------------------------------------------------------------------------------------------------- */

// Hurts and knocks Link back unless he is invincible, in a cutscene or not on the ground (when `groundOnly`).
s32 Eo_HurtPlayer(PlayState* play, Actor* source, u8 damage, s16 yaw, f32 knockback, s32 groundOnly, s32 shock);
// True while Link is standing on the ground (not jumping, sidehopping, backflipping, flying or climbing).
s32 Eo_PlayerOnGround(PlayState* play);
// A floor position near `pos` (NULL when there is no floor within reach).
s32 Eo_FindFloor(PlayState* play, Vec3f* pos, f32 maxDrop, f32* outY);
// A point `dist` behind Link on the floor, with no wall in between. False (and Link's position) when there is none.
s32 Eo_PosBehindPlayer(PlayState* play, f32 dist, Vec3f* outPos);
// A small puff of smoke, for summons that vanish.
void Eo_Puff(PlayState* play, Vec3f* pos, f32 scale);
// True when no cutscene is running and Link can be attacked.
s32 Eo_CanAttack(PlayState* play);

#endif
