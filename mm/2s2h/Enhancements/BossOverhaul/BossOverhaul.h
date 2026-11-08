#ifndef BOSS_OVERHAUL_H
#define BOSS_OVERHAUL_H

/**
 * Boss Overhaul (Enhancements > Difficulty Options > Boss Overhaul)
 *
 *  - Boss HP:           Odolwa, Goht, Gyorg and Twinmold have twice their health, every phase of Majora has three
 *                       times its health. Damage dealt by the player is unchanged.
 *  - Extra Attacks:     every boss gets new attacks built from vanilla actors, effects, sounds and display lists.
 *                       Vanilla idle / recovery / windup timings are never shortened.
 *  - Majora vs Fierce Deity: while Fierce Deity is worn, Majora takes half damage and fights harder.
 *
 * All three toggles are on by default and are read while the fight runs.
 */

#include "global.h"
#include <libultraship/bridge/consolevariablebridge.h>

#define BO_CVAR_HP "gEnhancements.BossOverhaul.BossHP"
#define BO_CVAR_ATTACKS "gEnhancements.BossOverhaul.ExtraAttacks"
#define BO_CVAR_FIERCE_DEITY "gEnhancements.BossOverhaul.MajoraVsFierceDeity"

#define BO_HP_ENABLED() (CVarGetInteger(BO_CVAR_HP, 1) != 0)
#define BO_ATTACKS_ENABLED() (CVarGetInteger(BO_CVAR_ATTACKS, 1) != 0)
#define BO_FIERCE_DEITY_ENABLED() (CVarGetInteger(BO_CVAR_FIERCE_DEITY, 1) != 0)

// Scales a vanilla health constant (or a threshold compared against health) for the temple bosses: 2x.
#define BO_HP(vanilla) (BO_HP_ENABLED() ? ((vanilla) * 2) : (vanilla))
// The same for every phase of Majora: 3x.
#define BO_HP3(vanilla) (BO_HP_ENABLED() ? ((vanilla) * 3) : (vanilla))

// True when `cur` is at or below half of a boss' (possibly scaled) maximum. `scaledMax` must already be scaled.
#define BO_AT_OR_BELOW_HALF(cur, scaledMax) ((s32)(cur) <= ((s32)(scaledMax) / 2))

/**
 * A simple glowing projectile that any boss can fire. It is an extra instance of the boss' own actor, spawned with
 * BO_ORB_PARAMS, so it never needs an object that isn't already loaded: it is drawn with gameplay_keep's light orb.
 */
#define BO_ORB_PARAMS 150

typedef enum BoOrbKind {
    /* 0 */ BO_ORB_FIRE,     // burns Link
    /* 1 */ BO_ORB_WATER,    // knocks Link back hard
    /* 2 */ BO_ORB_ELECTRIC, // shocks Link
    /* 3 */ BO_ORB_EMBER,    // a small, short lived puff of fire (fire breath, fire waves)
    /* 4 */ BO_ORB_KIND_MAX
} BoOrbKind;

typedef struct BoOrb {
    ColliderCylinder collider;
    s16 kind;
    s16 life;
    s16 maxLife;
    s16 spin;
    f32 gravity;     // added to velocity.y every frame
    f32 drag;        // velocity is multiplied by this every frame
    f32 drawScale;   // light orb scale
    f32 growth;      // drawScale grows by this every frame (fire breath spreads out)
    f32 sizeScale;   // multiplies collider and draw size (Twinmold uses it for Giant's Mask mode)
    u8 dieOnSurface; // see BoOrbSurfaceMode
} BoOrb;

typedef enum BoOrbSurfaceMode {
    /* 0 */ BO_ORB_DIE_NEVER,
    /* 1 */ BO_ORB_DIE_ON_ANY_SURFACE, // floor, wall or ceiling (default)
    /* 2 */ BO_ORB_DIE_ON_WALLS        // for flames that travel along the floor
} BoOrbSurfaceMode;

// Call from the boss' Init when params == BO_ORB_PARAMS. The kind comes from the spawn's rot.z.
void BoOrb_Init(Actor* actor, BoOrb* orb, PlayState* play);
void BoOrb_Update(Actor* actor, BoOrb* orb, PlayState* play);
void BoOrb_Draw(Actor* actor, BoOrb* orb, PlayState* play);
void BoOrb_Destroy(Actor* actor, BoOrb* orb, PlayState* play);

// Spawns an orb as an instance of `actorId` at `pos` moving with `vel`. Returns the actor (or NULL).
Actor* BoOrb_Spawn(PlayState* play, s16 actorId, BoOrbKind kind, Vec3f* pos, Vec3f* vel);

// Velocity that flies from `from` towards `to` at `speed`, turned sideways by `yawOffset`.
void BoOrb_AimVelocity(Vec3f* from, Vec3f* to, f32 speed, s16 yawOffset, Vec3f* outVel);
// Velocity for a lob from `from` that lands on `to` after `frames` frames under `gravity` (negative).
void BoOrb_ArcVelocity(Vec3f* from, Vec3f* to, f32 frames, f32 gravity, Vec3f* outVel);

// Majora vs Fierce Deity
s32 BoMM_IsFierceDeity(PlayState* play);
// Damage Majora takes: halved (never below 1) while Fierce Deity is worn and the option is on.
u8 BoMM_MajoraDamage(PlayState* play, u8 damage);

#endif
