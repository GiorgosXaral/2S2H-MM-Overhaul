/*
 * File: z_en_m_thunder.c
 * Overlay: ovl_En_M_Thunder
 * Description: Spin attack and sword beams
 */

#include "z_en_m_thunder.h"
#include "z64rumble.h"
#include "overlays/actors/ovl_Eff_Dust/z_eff_dust.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include <libultraship/bridge/consolevariablebridge.h>

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED)

void EnMThunder_Init(Actor* thisx, PlayState* play);
void EnMThunder_Destroy(Actor* thisx, PlayState* play);
void EnMThunder_Update(Actor* thisx, PlayState* play);
void EnMThunder_Draw(Actor* thisx, PlayState* play2);

void EnMThunder_UnkType_Update(Actor* thisx, PlayState* play);

void EnMThunder_AdjustLights(PlayState* play, f32 arg1);

void EnMThunder_Charge(EnMThunder* this, PlayState* play);
void EnMThunder_Spin_Attack(EnMThunder* this, PlayState* play);
void EnMThunder_SwordBeam_Attack(EnMThunder* this, PlayState* play);
void EnMThunder_UnkType_Attack(EnMThunder* this, PlayState* play);

#define ENMTHUNDER_TYPE_MAX 4

ActorProfile En_M_Thunder_Profile = {
    /**/ ACTOR_EN_M_THUNDER,
    /**/ ACTORCAT_ITEMACTION,
    /**/ FLAGS,
    /**/ GAMEPLAY_KEEP,
    /**/ sizeof(EnMThunder),
    /**/ EnMThunder_Init,
    /**/ EnMThunder_Destroy,
    /**/ EnMThunder_Update,
    /**/ EnMThunder_Draw,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK2,
        { 0x01000000, 0x00, 0x00 },
        { 0xF7CFFFFF, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NONE,
        ACELEM_ON,
        OCELEM_ON,
    },
    { 200, 200, 0, { 0, 0, 0 } },
};

static u8 sDamages[] = {
    1, 2, 3, 4, // Regular
    1, 2, 3, 4, // Great Spin
};

typedef enum {
    /* 0 */ ENMTHUNDER_SUBTYPE_SPIN_GREAT,
    /* 1 */ ENMTHUNDER_SUBTYPE_SPIN_REGULAR,
    /* 2 */ ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT,
    /* 3 */ ENMTHUNDER_SUBTYPE_SWORDBEAM_REGULAR,
    /* 4 */ ENMTHUNDER_SUBTYPE_MAX
} EnMThunderSubType;

static u16 sChargingSfxIds[ENMTHUNDER_SUBTYPE_MAX] = {
    NA_SE_IT_ROLLING_CUT_LV2, // ENMTHUNDER_SUBTYPE_SPIN_GREAT
    NA_SE_IT_ROLLING_CUT_LV1, // ENMTHUNDER_SUBTYPE_SPIN_REGULAR
    NA_SE_IT_ROLLING_CUT_LV2, // ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT
    NA_SE_IT_ROLLING_CUT_LV1, // ENMTHUNDER_SUBTYPE_SWORDBEAM_REGULAR
};

static f32 sScales[] = { 0.1f, 0.15f, 0.2f, 0.25f, 0.3f, 0.25f, 0.2f, 0.15f, 0.0f };

void EnMThunder_UnkType_Setup(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    this->actor.update = EnMThunder_UnkType_Update;
    this->isCharging = false;
    this->subtype = ENMTHUNDER_SUBTYPE_SPIN_REGULAR;
    this->scaleTarget = 2;
    this->actionFunc = EnMThunder_UnkType_Attack;
    this->timer = 8;
    this->lightColorFrac = 1.0f;
    AudioSfx_PlaySfx(NA_SE_IT_ROLLING_CUT_LV1, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                     &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    this->actor.child = NULL;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Sword projectiles (Enhancements > Items > Sword Projectiles)
 *
 *  - Gilded Sword, full hearts: a golden crescent. One hit for Gilded Sword slash damage, then it is gone. The same
 *    level of power as the Master Sword beam of the Ship of Harkinian build.
 *  - Great Fairy's Sword, full hearts: a spinning ring, slower and bigger, that cuts through every enemy in its path for
 *    Great Fairy's Sword slash damage.
 *  - Fierce Deity, any health: replaces the vanilla sword beam. Every normal slash (no lock-on needed) throws a deity
 *    orb for 1 magic, the fastest and hardest hitting of the three, which bursts on impact and hurts everything around.
 *
 * Normal slashes only; jump attacks and spin attacks stay vanilla. One projectile flies at a time. Everything is drawn
 * with gameplay_keep models (sword beam, spin attack disk, great spin, light orb), so no new assets are needed.
 * --------------------------------------------------------------------------------------------------------------- */

#define SWORD_SHOT_CVAR "gEnhancements.SwordProjectiles.Enabled"

typedef struct SwordShotInfo {
    u32 dmgFlags;
    u8 damage;
    s16 life;
    f32 speed;
    f32 scale;
    s16 radius;
    s16 height;
    Color_RGB8 prim;
    Color_RGB8 env;
} SwordShotInfo;

static SwordShotInfo sSwordShotInfo[ENMTHUNDER_SHOT_MAX] = {
    // Gilded Sword: Gilded Sword slash damage (3), one hit
    { DMG_SWORD, 3, 22, 28.0f, 7.0f, 34, 40, { 255, 240, 150 }, { 255, 150, 0 } },
    // Great Fairy's Sword: Great Fairy's Sword slash damage (4), pierces
    { DMG_SWORD, 4, 30, 18.0f, 1.6f, 48, 36, { 255, 170, 240 }, { 220, 40, 180 } },
    // Fierce Deity orb: 6 (the vanilla deity beam does 3), bursts on impact
    { DMG_SWORD_BEAM, 6, 24, 36.0f, 7.0f, 30, 40, { 230, 255, 255 }, { 0, 200, 255 } },
    // Fierce Deity burst: 4 to everything around the impact
    { DMG_SWORD_BEAM, 4, 14, 0.0f, 3.0f, 90, 80, { 200, 255, 255 }, { 0, 120, 255 } },
};

static ColliderCylinderInit sSwordShotCylinderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_PLAYER,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK2,
        { DMG_SWORD, 0x00, 0x03 },
        { 0xF7CFFFFF, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 30, 40, -20, { 0, 0, 0 } },
};

s32 EnMThunder_DeityShotReplacesBeam(void) {
    return CVarGetInteger(SWORD_SHOT_CVAR, 1) != 0;
}

static s32 EnMThunder_SwordShotFlying(PlayState* play) {
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first;

    while (actor != NULL) {
        if ((actor->id == ACTOR_EN_M_THUNDER) && (actor->update != NULL) && ENMTHUNDER_IS_SWORD_SHOT(actor) &&
            (actor->params != ENMTHUNDER_SWORD_SHOT_PARAMS(ENMTHUNDER_SHOT_DEITY_BURST))) {
            return true;
        }
        actor = actor->next;
    }
    return false;
}

static Actor* EnMThunder_SpawnSwordShot(PlayState* play, s32 kind, Vec3f* pos, s16 pitch, s16 yaw) {
    return Actor_Spawn(&play->actorCtx, play, ACTOR_EN_M_THUNDER, pos->x, pos->y, pos->z, pitch, yaw, 0,
                       ENMTHUNDER_SWORD_SHOT_PARAMS(kind));
}

void EnMThunder_TrySwordShot(PlayState* play, Player* player) {
    Actor* shot;
    s32 kind;
    Vec3f pos;
    s16 pitch = 0;
    s16 yaw;
    s32 fullHealth;

    if ((play == NULL) || (player == NULL) || (player->actor.id != ACTOR_PLAYER)) {
        return;
    }
    if (!CVarGetInteger(SWORD_SHOT_CVAR, 1)) {
        return;
    }
    // Normal slashes, stabs and combo finishers only: jump attacks and spin attacks stay vanilla
    if (!((player->meleeWeaponAnimation <= PLAYER_MWA_STAB_COMBO_2H) ||
          (player->meleeWeaponAnimation == PLAYER_MWA_BACKSLASH_RIGHT) ||
          (player->meleeWeaponAnimation == PLAYER_MWA_BACKSLASH_LEFT))) {
        return;
    }

    fullHealth = gSaveContext.save.saveInfo.playerData.health >= gSaveContext.save.saveInfo.playerData.healthCapacity;

    if (player->transformation == PLAYER_FORM_FIERCE_DEITY) {
        kind = ENMTHUNDER_SHOT_DEITY;
    } else if ((player->transformation == PLAYER_FORM_HUMAN) && (player->heldItemAction == PLAYER_IA_SWORD_GILDED) &&
               fullHealth) {
        kind = ENMTHUNDER_SHOT_GILDED;
    } else if ((player->transformation == PLAYER_FORM_HUMAN) &&
               (player->heldItemAction == PLAYER_IA_SWORD_TWO_HANDED) && fullHealth) {
        kind = ENMTHUNDER_SHOT_GREAT_FAIRY;
    } else {
        return;
    }

    if (EnMThunder_SwordShotFlying(play)) {
        return;
    }

    // The deity orb costs 1 magic, like the vanilla deity beam it replaces
    if ((kind == ENMTHUNDER_SHOT_DEITY) &&
        ((gSaveContext.save.saveInfo.playerData.magic == 0) ||
         ((gSaveContext.magicState != MAGIC_STATE_IDLE) && (gSaveContext.magicState != MAGIC_STATE_CONSUME_LENS)))) {
        return;
    }

    yaw = player->actor.shape.rot.y;
    pos = player->bodyPartsPos[PLAYER_BODYPART_WAIST];
    pos.x += Math_SinS(yaw) * 25.0f;
    pos.y += 12.0f;
    pos.z += Math_CosS(yaw) * 25.0f;

    // When locked on, aim up or down at the target (the ring tilts with the shot)
    if (player->focusActor != NULL) {
        pitch = Math_Vec3f_Pitch(&pos, &player->focusActor->focus.pos);
        pitch = CLAMP(pitch, -0x2000, 0x2000);
    }

    shot = EnMThunder_SpawnSwordShot(play, kind, &pos, pitch, yaw);

    // Magic is only spent once the orb exists (like the vanilla beam)
    if ((shot != NULL) && (kind == ENMTHUNDER_SHOT_DEITY) && !Magic_Consume(play, 1, MAGIC_CONSUME_DEITY_BEAM)) {
        Actor_Kill(shot);
    }
}

static void EnMThunder_SwordShotInit(EnMThunder* this, PlayState* play) {
    SwordShotInfo* info;
    f32 speed;

    this->shotKind = this->actor.params - ENMTHUNDER_SWORD_SHOT_PARAMS(0);
    info = &sSwordShotInfo[this->shotKind];
    speed = info->speed;

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sSwordShotCylinderInit);
    this->collider.elem.atDmgInfo.dmgFlags = info->dmgFlags;
    this->collider.elem.atDmgInfo.damage = info->damage;
    this->collider.dim.radius = info->radius;
    this->collider.dim.height = info->height;
    this->collider.dim.yShift = -info->height / 2;

    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, info->prim.r, info->prim.g, info->prim.b, 180);
    this->lightNode = LightContext_InsertLight(play, &play->lightCtx, &this->lightInfo);

    this->actor.velocity.x = Math_CosS(this->actor.world.rot.x) * Math_SinS(this->actor.world.rot.y) * speed;
    this->actor.velocity.y = -Math_SinS(this->actor.world.rot.x) * speed;
    this->actor.velocity.z = Math_CosS(this->actor.world.rot.x) * Math_CosS(this->actor.world.rot.y) * speed;

    // The sword beam and spin attack models face backwards, like the vanilla ones
    this->actor.shape.rot.y = this->actor.world.rot.y + 0x8000;
    this->actor.shape.rot.x = -this->actor.world.rot.x;
    this->actor.shape.rot.z = 0;
    this->actor.room = -1;
    this->actor.flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    this->shotTimer = info->life;
    this->shotCooldown = 0;
    this->shotSpin = 0;
    this->isCharging = false;
    this->alphaFrac = 1.0f;
    this->lightColorFrac = 1.0f;
    // Counts as a sword beam for other code (Fierce Deity Anywhere keeps sword beams from hitting non-enemies)
    this->subtype = ((this->shotKind == ENMTHUNDER_SHOT_DEITY) || (this->shotKind == ENMTHUNDER_SHOT_DEITY_BURST))
                        ? ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT
                        : ENMTHUNDER_SUBTYPE_SPIN_GREAT;

    if (this->shotKind == ENMTHUNDER_SHOT_DEITY_BURST) {
        Actor_SetScale(&this->actor, 0.1f);
        Actor_PlaySfx(&this->actor, NA_SE_IT_ROLLING_CUT_LV2);
        Rumble_Request(0.0f, 180, 10, 100);
    } else {
        Actor_SetScale(&this->actor, info->scale);
        Actor_PlaySfx(&this->actor,
                      (this->shotKind == ENMTHUNDER_SHOT_GILDED) ? NA_SE_IT_ROLLING_CUT_LV1 : NA_SE_IT_ROLLING_CUT_LV2);
    }
}

// impact: hit something (a spark), rather than running out of range (just vanishes). The deity orb always bursts.
static void EnMThunder_SwordShotEnd(EnMThunder* this, PlayState* play, s32 impact) {
    if (this->shotKind == ENMTHUNDER_SHOT_DEITY) {
        EnMThunder_SpawnSwordShot(play, ENMTHUNDER_SHOT_DEITY_BURST, &this->actor.world.pos, 0, 0);
    } else if (impact && (this->shotKind != ENMTHUNDER_SHOT_DEITY_BURST)) {
        EffectSsHitmark_SpawnFixedScale(play, 0, &this->actor.world.pos);
    }
    Actor_Kill(&this->actor);
}

static void EnMThunder_SwordShotUpdate(EnMThunder* this, PlayState* play) {
    static Vec3f sZeroVec = { 0.0f, 0.0f, 0.0f };
    SwordShotInfo* info = &sSwordShotInfo[this->shotKind];
    Color_RGBA8 prim = { 255, 255, 255, 255 };
    Color_RGBA8 env = { info->env.r, info->env.g, info->env.b, 0 };
    Vec3f sparklePos;
    s32 registerAT = true;

    if (Play_InCsMode(play)) {
        Actor_Kill(&this->actor);
        return;
    }

    if (this->shotTimer <= 0) {
        EnMThunder_SwordShotEnd(this, play, false);
        return;
    }
    this->shotTimer--;

    if (this->shotKind == ENMTHUNDER_SHOT_DEITY_BURST) {
        // Grows fast, then fades. Only hurts while it grows, so each enemy is hit once.
        Math_SmoothStepToF(&this->actor.scale.x, info->scale, 0.5f, 0.6f, 0.0f);
        Actor_SetScale(&this->actor, this->actor.scale.x);
        this->collider.dim.radius = this->actor.scale.x * 30.0f;
        this->alphaFrac = CLAMP_MAX(this->shotTimer / 8.0f, 1.0f);
        this->lightColorFrac = this->alphaFrac;
        registerAT = this->shotTimer >= (info->life - 6);
        this->shotSpin += 0x800;
    } else {
        // Anything the projectile touched last frame (an enemy, a shield, a switch)
        if (this->collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
            this->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
            if (this->shotKind != ENMTHUNDER_SHOT_GREAT_FAIRY) {
                EnMThunder_SwordShotEnd(this, play, true);
                return;
            }
            // The ring cuts through: stop hurting for a few frames so each enemy is hit once
            this->shotCooldown = 6;
        }

        Actor_UpdatePos(&this->actor);
        // The ring only stops at the floor when it was aimed downwards, so that it skims over uneven ground
        Actor_UpdateBgCheckInfo(play, &this->actor, 15.0f, 15.0f, 15.0f,
                                ((this->shotKind == ENMTHUNDER_SHOT_GREAT_FAIRY) && (this->actor.world.rot.x < 0x200))
                                    ? (UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2)
                                    : (UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_4));
        if (this->actor.bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WALL | BGCHECKFLAG_CEILING)) {
            EnMThunder_SwordShotEnd(this, play, true);
            return;
        }

        if (this->shotTimer < 6) {
            this->alphaFrac = this->shotTimer / 6.0f;
        }
        if (this->shotCooldown > 0) {
            this->shotCooldown--;
            registerAT = false;
        }
        this->shotSpin += (this->shotKind == ENMTHUNDER_SHOT_DEITY) ? 0x1800 : 0x1000;

        sparklePos.x = this->actor.world.pos.x + Rand_CenteredFloat(info->radius);
        sparklePos.y = this->actor.world.pos.y + Rand_CenteredFloat(20.0f);
        sparklePos.z = this->actor.world.pos.z + Rand_CenteredFloat(info->radius);
        EffectSsKirakira_SpawnSmall(play, &sparklePos, &sZeroVec, &sZeroVec, &prim, &env);
    }

    this->scroll += 1.0f;

    Collider_UpdateCylinder(&this->actor, &this->collider);
    if (this->shotKind == ENMTHUNDER_SHOT_GILDED) {
        // The sword beam model sits ahead of the actor, like the vanilla beam's collider
        this->collider.dim.pos.x += Math_SinS(this->actor.world.rot.y) * 5.0f * this->actor.scale.x;
        this->collider.dim.pos.z += Math_CosS(this->actor.world.rot.y) * 5.0f * this->actor.scale.x;
    }
    if (registerAT) {
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }

    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, info->prim.r * this->lightColorFrac,
                              info->prim.g * this->lightColorFrac, info->prim.b * this->lightColorFrac,
                              (this->shotKind == ENMTHUNDER_SHOT_DEITY_BURST) ? 400 : 180);
}

static void EnMThunder_SwordShotDraw(EnMThunder* this, PlayState* play) {
    SwordShotInfo* info = &sSwordShotInfo[this->shotKind];
    u8 alpha = (u8)(this->alphaFrac * 255.0f);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    switch (this->shotKind) {
        case ENMTHUNDER_SHOT_GILDED:
            // The vanilla sword beam model, smaller and golden (the actor matrix is already set up)
            Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 16, 64, 1, 0,
                                          0x1FF - ((u16)(s32)(this->scroll * 10.0f) & 0x1FF), 32, 128, 0, 0, 0, -10));
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, info->prim.r, info->prim.g, info->prim.b, alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, info->env.r, info->env.g, info->env.b, 128);
            gSPDisplayList(POLY_XLU_DISP++, gSwordBeamDL);
            break;

        case ENMTHUNDER_SHOT_GREAT_FAIRY:
        case ENMTHUNDER_SHOT_DEITY_BURST:
            // A spin attack wave flying on its own (Great Fairy) / a great spin wave bursting out (Fierce Deity)
            Matrix_RotateYS(this->shotSpin, MTXMODE_APPLY);
            Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0xFF - ((u16)(s32)(this->scroll * 30.0f) & 0xFF), 0,
                                          64, 32, 1, 0xFF - ((u16)(s32)(this->scroll * 20.0f) & 0xFF), 0, 8, 8, -30, 0,
                                          -20, 0));
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, info->prim.r, info->prim.g, info->prim.b, alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, info->env.r, info->env.g, info->env.b, 128);
            if (this->shotKind == ENMTHUNDER_SHOT_GREAT_FAIRY) {
                gSPDisplayList(POLY_XLU_DISP++, gSpinAttackDiskDL);
            } else {
                gSPDisplayList(POLY_XLU_DISP++, gGreatSpinAttackDiskDL);
                gSPDisplayList(POLY_XLU_DISP++, gGreatSpinAttackCylinderDL);
            }
            break;

        case ENMTHUNDER_SHOT_DEITY:
            // A big spinning orb of light
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, info->prim.r, info->prim.g, info->prim.b, alpha);
            gDPSetEnvColor(POLY_XLU_DISP++, info->env.r, info->env.g, info->env.b, 128);
            Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
            Matrix_Scale(info->scale, info->scale, 0.0f, MTXMODE_APPLY);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_RotateZS(this->shotSpin, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);
            gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);
            // with a smaller white core spinning the other way
            gDPPipeSync(POLY_XLU_DISP++);
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, alpha);
            Matrix_Translate(this->actor.world.pos.x, this->actor.world.pos.y, this->actor.world.pos.z, MTXMODE_NEW);
            Matrix_Scale(info->scale * 0.55f, info->scale * 0.55f, 0.0f, MTXMODE_APPLY);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_RotateZS(-this->shotSpin * 2, MTXMODE_APPLY);
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);
            break;

        default:
            break;
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void EnMThunder_Init(Actor* thisx, PlayState* play) {
    s32 pad;
    EnMThunder* this = (EnMThunder*)thisx;
    Player* player = GET_PLAYER(play);

    if (ENMTHUNDER_IS_SWORD_SHOT(&this->actor)) {
        EnMThunder_SwordShotInit(this, play);
        return;
    }

    Collider_InitCylinder(play, &this->collider);
    Collider_SetCylinder(play, &this->collider, &this->actor, &sCylinderInit);
    this->type = ENMTHUNDER_GET_TYPE(&this->actor);
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, 255, 255, 255, 0);
    this->lightNode = LightContext_InsertLight(play, &play->lightCtx, &this->lightInfo);

    if (this->type == ENMTHUNDER_TYPE_UNK) {
        EnMThunder_UnkType_Setup(this, play);
        return;
    }

    this->collider.dim.radius = 0;
    this->collider.dim.height = 40;
    this->collider.dim.yShift = -20;
    this->timer = 8;
    this->scroll = 0.0f;
    this->actor.world.pos = player->bodyPartsPos[PLAYER_BODYPART_WAIST];
    this->lightColorFrac = 0.0f;
    this->adjustLightsArg1 = 0.0f;
    this->actor.shape.rot.y = player->actor.shape.rot.y + 0x8000;
    this->actor.shape.rot.x = -this->actor.world.rot.x;
    this->actor.room = -1;
    Actor_SetScale(&this->actor, 0.1f);
    this->isCharging = false;

    if (player->stateFlags2 & PLAYER_STATE2_20000) {
        if (!gSaveContext.save.saveInfo.playerData.isMagicAcquired || (gSaveContext.magicState != MAGIC_STATE_IDLE) ||
            ((ENMTHUNDER_GET_MAGIC_COST(&this->actor) != 0) &&
             !Magic_Consume(play, ENMTHUNDER_GET_MAGIC_COST(&this->actor), MAGIC_CONSUME_NOW))) {
            AudioSfx_PlaySfx(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            AudioSfx_PlaySfx(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            Actor_Kill(&this->actor);
            return;
        }

        player->stateFlags2 &= ~PLAYER_STATE2_20000;
        this->isCharging = false;

        if (CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK)) {
            player->unk_B08 = 1.0f;
            this->collider.elem.atDmgInfo.damage = sDamages[this->type + ENMTHUNDER_TYPE_MAX];
            this->subtype = ENMTHUNDER_SUBTYPE_SPIN_GREAT;
            if (this->type == ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD) {
                this->scaleTarget = 6;
            } else if (this->type == ENMTHUNDER_TYPE_GILDED_SWORD) {
                this->scaleTarget = 4;
            } else {
                this->scaleTarget = 3;
            }
        } else {
            player->unk_B08 = 0.5f;
            this->collider.elem.atDmgInfo.damage = sDamages[this->type];
            this->subtype = ENMTHUNDER_SUBTYPE_SPIN_REGULAR;
            if (this->type == ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD) {
                this->scaleTarget = 4;
            } else if (this->type == ENMTHUNDER_TYPE_GILDED_SWORD) {
                this->scaleTarget = 3;
            } else {
                this->scaleTarget = 2;
            }
        }

        if (player->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
            this->subtype += ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT;
            this->actionFunc = EnMThunder_SwordBeam_Attack;
            this->timer = 1;
            this->scaleTarget = 12;
            this->collider.elem.atDmgInfo.dmgFlags = DMG_SWORD_BEAM;
            this->collider.elem.atDmgInfo.damage = 3;
        } else {
            this->actionFunc = EnMThunder_Spin_Attack;
            this->timer = 8;
        }

        AudioSfx_PlaySfx(NA_SE_IT_ROLLING_CUT_LV1, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                         &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);

        this->lightColorFrac = 1.0f;
    } else {
        this->actionFunc = EnMThunder_Charge;
    }

    this->actor.child = NULL;
}

void EnMThunder_Destroy(Actor* thisx, PlayState* play) {
    EnMThunder* this = (EnMThunder*)thisx;

    if (ENMTHUNDER_IS_SWORD_SHOT(&this->actor)) {
        Collider_DestroyCylinder(play, &this->collider);
        LightContext_RemoveLight(play, &play->lightCtx, this->lightNode);
        return;
    }

    if (this->isCharging) {
        Magic_Reset(play);
    }

    Collider_DestroyCylinder(play, &this->collider);
    EnMThunder_AdjustLights(play, 0.0f);
    LightContext_RemoveLight(play, &play->lightCtx, this->lightNode);
}

void EnMThunder_AdjustLights(PlayState* play, f32 arg1) {
    Environment_AdjustLights(play, arg1, 850.0f, 0.2f, 0.0f);
}

void EnMThunder_Spin_AttackNoMagic(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (player->stateFlags2 & PLAYER_STATE2_20000) {
        if (player->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H) {
            AudioSfx_PlaySfx(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            AudioSfx_PlaySfx(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                             &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
        }
        Actor_Kill(&this->actor);
        return;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK)) {
        Actor_Kill(&this->actor);
    }
}

void EnMThunder_Charge(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Actor* child = this->actor.child;

    this->unk1B0 = player->unk_B08;
    this->actor.world.pos = player->bodyPartsPos[PLAYER_BODYPART_WAIST];
    this->actor.shape.rot.y = player->actor.shape.rot.y + 0x8000;

    if (!this->isCharging && (player->unk_B08 >= 0.1f)) {
        if ((gSaveContext.magicState != MAGIC_STATE_IDLE) ||
            ((ENMTHUNDER_GET_MAGIC_COST(&this->actor) != 0) &&
             !Magic_Consume(play, ENMTHUNDER_GET_MAGIC_COST(&this->actor), MAGIC_CONSUME_WAIT_PREVIEW))) {
            EnMThunder_Spin_AttackNoMagic(this, play);
            this->actionFunc = EnMThunder_Spin_AttackNoMagic;
            this->chargingAlpha = 0;
            this->adjustLightsArg1 = 0.0f;
            this->lightColorFrac = 0.0f;
            return;
        }
        this->isCharging = true;
    }

    if (player->unk_B08 >= 0.1f) {
        Rumble_Request(0.0f, (s32)(player->unk_B08 * 150.0f), 2, (s32)(player->unk_B08 * 150.0f));
    }

    if (player->stateFlags2 & PLAYER_STATE2_20000) {
        if ((child != NULL) && (child->update != NULL)) {
            child->parent = NULL;
        }

        if (player->unk_B08 <= 0.15f) {
            if ((player->unk_B08 >= 0.1f) && (player->meleeWeaponAnimation >= PLAYER_MWA_SPIN_ATTACK_1H)) {
                AudioSfx_PlaySfx(NA_SE_IT_ROLLING_CUT, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
                AudioSfx_PlaySfx(NA_SE_IT_SWORD_SWING_HARD, &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                                 &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
            Actor_Kill(&this->actor);
            return;
        }

        player->stateFlags2 &= ~PLAYER_STATE2_20000;

        if (ENMTHUNDER_GET_MAGIC_COST(&this->actor) != 0) {
            gSaveContext.magicState = MAGIC_STATE_CONSUME_SETUP;
        }

        if (player->unk_B08 < 0.85f) {
            this->collider.elem.atDmgInfo.damage = sDamages[this->type];
            this->subtype = ENMTHUNDER_SUBTYPE_SPIN_REGULAR;
            if (this->type == ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD) {
                this->scaleTarget = 4;
            } else if (this->type == ENMTHUNDER_TYPE_GILDED_SWORD) {
                this->scaleTarget = 3;
            } else {
                this->scaleTarget = 2;
            }
        } else {
            this->collider.elem.atDmgInfo.damage = sDamages[this->type + ENMTHUNDER_TYPE_MAX];
            this->subtype = ENMTHUNDER_SUBTYPE_SPIN_GREAT;
            if (this->type == ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD) {
                this->scaleTarget = 6;
            } else if (this->type == ENMTHUNDER_TYPE_GILDED_SWORD) {
                this->scaleTarget = 4;
            } else {
                this->scaleTarget = 3;
            }
        }

        if (player->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) {
            this->subtype += ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT;
            this->actionFunc = EnMThunder_SwordBeam_Attack;
            this->timer = 1;
        } else {
            this->actionFunc = EnMThunder_Spin_Attack;
            this->timer = 8;
        }

        AudioSfx_PlaySfx(sChargingSfxIds[this->subtype], &player->actor.projectedPos, 4, &gSfxDefaultFreqAndVolScale,
                         &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);

        this->lightColorFrac = 1.0f;

        return;
    }

    if (!(player->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK)) {
        if (this->actor.child != NULL) {
            this->actor.child->parent = NULL;
        }
        Actor_Kill(&this->actor);
        return;
    }

    if (player->unk_B08 > 0.15f) {
        this->chargingAlpha = 255;
        if (this->actor.child == NULL) {
            Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_EFF_DUST, this->actor.world.pos.x,
                               this->actor.world.pos.y, this->actor.world.pos.z, 0, this->actor.shape.rot.y, 0,
                               EFF_DUST_TYPE_SPIN_ATTACK_CHARGE);
        }
        this->adjustLightsArg1 += (((player->unk_B08 - 0.15f) * 1.5f) - this->adjustLightsArg1) * 0.5f;
    } else if (player->unk_B08 > .1f) {
        this->chargingAlpha = (s32)((player->unk_B08 - .1f) * 255.0f * 20.0f);
        this->lightColorFrac = (player->unk_B08 - .1f) * 10.0f;
    } else {
        this->chargingAlpha = 0;
    }

    if (player->unk_B08 > 0.85f) {
        Audio_PlaySfx_SwordCharge(&player->actor.projectedPos, 2);
    } else if (player->unk_B08 > 0.15f) {
        Audio_PlaySfx_SwordCharge(&player->actor.projectedPos, 1);
    } else if (player->unk_B08 > 0.1f) {
        Audio_PlaySfx_SwordCharge(&player->actor.projectedPos, 0);
    }

    if (Play_InCsMode(play)) {
        Actor_Kill(&this->actor);
    }
}

void func_808B5EEC(EnMThunder* this, PlayState* play) {
    if (this->timer < 2) {
        if (this->chargingAlpha < 40) {
            this->chargingAlpha = 0;
        } else {
            this->chargingAlpha -= 40;
        }
    }

    this->scroll += 2.0f * this->alphaFrac;

    if (this->adjustLightsArg1 < this->lightColorFrac) {
        this->adjustLightsArg1 = F32_LERPIMP(this->adjustLightsArg1, this->lightColorFrac, 0.1f);
    } else {
        this->adjustLightsArg1 = this->lightColorFrac;
    }
}

void EnMThunder_Spin_Attack(EnMThunder* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (Math_StepToF(&this->lightColorFrac, 0.0f, 0.0625f)) {
        Actor_Kill(&this->actor);
    } else {
        Math_SmoothStepToF(&this->actor.scale.x, (s32)this->scaleTarget, 0.6f, 0.8f, 0.0f);
        Actor_SetScale(&this->actor, this->actor.scale.x);
        this->collider.dim.radius = this->actor.scale.x * 30.0f;
        Collider_UpdateCylinder(&this->actor, &this->collider);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }

    if (this->timer > 0) {
        this->actor.world.pos.x = player->bodyPartsPos[PLAYER_BODYPART_WAIST].x;
        this->actor.world.pos.z = player->bodyPartsPos[PLAYER_BODYPART_WAIST].z;
        this->timer--;
    }

    if (this->lightColorFrac > (6.0f / 10.0f)) {
        this->alphaFrac = 1.0f;
    } else {
        this->alphaFrac = this->lightColorFrac * (10.0f / 6.0f);
    }

    func_808B5EEC(this, play);

    if (Play_InCsMode(play)) {
        Actor_Kill(&this->actor);
    }
}

void EnMThunder_SwordBeam_Attack(EnMThunder* this, PlayState* play) {
    s32 pad[2];
    f32 sp2C;

    if (this->lightColorFrac > (9.0f / 10.0f)) {
        this->alphaFrac = 1.0f;
    } else {
        this->alphaFrac = this->lightColorFrac * (10.0f / 9.0f);
    }

    if (Math_StepToF(&this->lightColorFrac, 0.0f, 0.05f)) {
        Actor_Kill(&this->actor);
    } else {
        sp2C = -80.0f * Math_CosS(this->actor.world.rot.x);

        this->actor.world.pos.x += sp2C * Math_SinS(this->actor.shape.rot.y);
        this->actor.world.pos.z += sp2C * Math_CosS(this->actor.shape.rot.y);
        this->actor.world.pos.y += -80.0f * Math_SinS(this->actor.world.rot.x);

        Math_SmoothStepToF(&this->actor.scale.x, this->scaleTarget, 0.6f, 2.0f, 0.0f);
        Actor_SetScale(&this->actor, this->actor.scale.x);

        this->collider.dim.radius = this->actor.scale.x * 5.0f;

        this->collider.dim.pos.x = this->actor.world.pos.x;
        this->collider.dim.pos.y = this->actor.world.pos.y;
        this->collider.dim.pos.z = this->actor.world.pos.z;

        this->collider.dim.pos.x =
            (Math_SinS(this->actor.shape.rot.y) * -5.0f * this->actor.scale.x) + this->actor.world.pos.x;
        this->collider.dim.pos.y = this->actor.world.pos.y;
        this->collider.dim.pos.z =
            (Math_CosS(this->actor.shape.rot.y) * -5.0f * this->actor.scale.z) + this->actor.world.pos.z;

        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }

    if (this->timer > 0) {
        this->timer--;
    }

    func_808B5EEC(this, play);
}

void EnMThunder_UnkType_Attack(EnMThunder* this, PlayState* play) {
    if (Math_StepToF(&this->lightColorFrac, 0.0f, 0.0625f)) {
        Actor_Kill(&this->actor);
    } else {
        Math_SmoothStepToF(&this->actor.scale.x, (s32)this->scaleTarget, 0.6f, 0.8f, 0.0f);
        Actor_SetScale(&this->actor, this->actor.scale.x);
    }

    if (this->lightColorFrac > (6.0f / 10.0f)) {
        this->alphaFrac = 1.0f;
    } else {
        this->alphaFrac = this->lightColorFrac * (10.0f / 6.0f);
    }

    func_808B5EEC(this, play);
}

void EnMThunder_Update(Actor* thisx, PlayState* play) {
    EnMThunder* this = (EnMThunder*)thisx;

    if (ENMTHUNDER_IS_SWORD_SHOT(&this->actor)) {
        EnMThunder_SwordShotUpdate(this, play);
        return;
    }

    this->actionFunc(this, play);
    EnMThunder_AdjustLights(play, this->adjustLightsArg1);
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, this->lightColorFrac * 255.0f, this->lightColorFrac * 255.0f,
                              this->lightColorFrac * 100.0f, this->lightColorFrac * 800.0f);
}

void EnMThunder_UnkType_Update(Actor* thisx, PlayState* play) {
    EnMThunder* this = (EnMThunder*)thisx;

    this->actionFunc(this, play);
    Lights_PointNoGlowSetInfo(&this->lightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                              this->actor.world.pos.z, this->lightColorFrac * 255.0f, this->lightColorFrac * 255.0f,
                              this->lightColorFrac * 100.0f, this->lightColorFrac * 800.0f);
}

void EnMThunder_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    EnMThunder* this = (EnMThunder*)thisx;
    Player* player = GET_PLAYER(play);
    f32 scale;
    s32 y2Scroll;

    if (ENMTHUNDER_IS_SWORD_SHOT(&this->actor)) {
        EnMThunder_SwordShotDraw(this, play);
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);

    switch (this->subtype) {
        case ENMTHUNDER_SUBTYPE_SPIN_GREAT:
        case ENMTHUNDER_SUBTYPE_SPIN_REGULAR:
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0xFF - ((u16)(s32)(this->scroll * 30.0f) & 0xFF), 0,
                                          64, 32, 1, 0xFF - ((u16)(s32)(this->scroll * 20.0f) & 0xFF), 0, 8, 8, -30, 0,
                                          -20, 0));
            break;

        case ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT:
        case ENMTHUNDER_SUBTYPE_SWORDBEAM_REGULAR:
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 16, 64, 1, 0,
                                          0x1FF - ((u16)(s32)(this->scroll * 10.0f) & 0x1FF), 32, 128, 0, 0, 0, -10));
            break;

        default:
            break;
    }

    switch (this->subtype) {
        case ENMTHUNDER_SUBTYPE_SPIN_GREAT:
            gDPSetPrimColorOverride(POLY_XLU_DISP++, 0, 0x80, 255, 255, 170, (u16)(this->alphaFrac * 255.0f),
                                    COSMETIC_ID("Effects.GreatSpinBurst"));
            gSPDisplayList(POLY_XLU_DISP++, gGreatSpinAttackDiskDL);
            gSPDisplayList(POLY_XLU_DISP++, gGreatSpinAttackCylinderDL);
            break;

        case ENMTHUNDER_SUBTYPE_SPIN_REGULAR:
            gDPSetPrimColorOverride(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, (u16)(this->alphaFrac * 255.0f),
                                    COSMETIC_ID("Effects.SpinSlashBurst"));
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttackDiskDL);
            gSPDisplayList(POLY_XLU_DISP++, gSpinAttackCylinderDL);
            break;

        case ENMTHUNDER_SUBTYPE_SWORDBEAM_REGULAR:
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, (u16)(this->alphaFrac * 255.0f));
            gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, 128);
            gSPDisplayList(POLY_XLU_DISP++, gSwordBeamDL);
            break;

        case ENMTHUNDER_SUBTYPE_SWORDBEAM_GREAT:
            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 0, 255, 255, (u16)(this->alphaFrac * 255.0f));
            gDPSetEnvColor(POLY_XLU_DISP++, 200, 200, 200, 128);
            gSPDisplayList(POLY_XLU_DISP++, gSwordBeamDL);
            break;

        default:
            break;
    }

    Matrix_Mult(&player->leftHandMf, MTXMODE_NEW);

    if (GameInteractor_Should(VB_TRANSFORM_THUNDER_MATRIX, true, this)) {
        if (this->type == ENMTHUNDER_TYPE_GILDED_SWORD) {
            Matrix_Translate(0.0f, 220.0f, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(-1.2f, -0.8f, -0.6f, MTXMODE_APPLY);
            Matrix_RotateXS(0x4000, MTXMODE_APPLY);
        } else {
            Matrix_Translate(0.0f, 220.0f, 0.0f, MTXMODE_APPLY);
            Matrix_Scale(-0.7f, -0.6f, -0.4f, MTXMODE_APPLY);
            Matrix_RotateXS(0x4000, MTXMODE_APPLY);
        }
    }

    if (this->unk1B0 >= 0.85f) {
        scale = (sScales[play->gameplayFrames & 7] * 6.0f) + 1.0f;
        gDPSetPrimColorOverride(POLY_XLU_DISP++, 0, 0x80, 255, 255, 170, this->chargingAlpha,
                                COSMETIC_ID("Effects.GreatSpinCharge"));
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 100, 0, 128);
        y2Scroll = 40;
    } else {
        scale = (sScales[play->gameplayFrames & 7] * 2.0f) + 1.0f;
        gDPSetPrimColorOverride(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, this->chargingAlpha,
                                COSMETIC_ID("Effects.SpinSlashCharge"));
        gDPSetEnvColor(POLY_XLU_DISP++, 0, 100, 255, 128);
        y2Scroll = 20;
    }

    Matrix_Scale(1.0f, scale, scale, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x09,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, (play->gameplayFrames * 5) & 0xFF, 0, 32, 32, 1,
                                  (play->gameplayFrames * 20) & 0xFF, (play->gameplayFrames * y2Scroll) & 0xFF, 8, 8, 5,
                                  0, 20, y2Scroll));
    gSPDisplayList(POLY_XLU_DISP++, gSpinAttackChargingDL);

    CLOSE_DISPS(play->state.gfxCtx);
}
