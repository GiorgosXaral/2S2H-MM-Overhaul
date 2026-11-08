#include "BossOverhaul.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

/**
 * Boss Overhaul: shared projectile ("orb") and Majora helpers. See BossOverhaul.h.
 *
 * An orb is an extra instance of a boss' own actor. The boss' Init hands it to BoOrb_Init when the params are
 * BO_ORB_PARAMS, and its update / draw / destroy forward to the functions below. It only draws gameplay_keep's light
 * orb (plus gameplay_keep fire sprites for the fiery kinds), both of which are loaded in every scene, so an orb can't
 * depend on an object that happens to be missing from a boss room.
 */

typedef struct BoOrbKindInfo {
    u8 atEffect; // 1 = burn, 2 = freeze, 3 = shock, 4 = big knockback (see Player's damage response)
    u8 damage;   // 0x10 is one heart
    s16 radius;
    s16 height;
    s16 life;
    f32 drawScale;
    f32 growth;
    Color_RGB8 prim;
    Color_RGB8 env;
} BoOrbKindInfo;

static BoOrbKindInfo sBoOrbKindInfo[BO_ORB_KIND_MAX] = {
    /* BO_ORB_FIRE     */ { 1, 0x10, 22, 44, 120, 4.0f, 0.0f, { 255, 220, 120 }, { 255, 60, 0 } },
    /* BO_ORB_WATER    */ { 4, 0x10, 22, 44, 120, 4.0f, 0.0f, { 210, 245, 255 }, { 40, 120, 255 } },
    /* BO_ORB_ELECTRIC */ { 3, 0x10, 22, 44, 150, 3.5f, 0.0f, { 255, 255, 200 }, { 120, 160, 255 } },
    /* BO_ORB_EMBER    */ { 1, 0x08, 18, 36, 26, 2.0f, 0.12f, { 255, 200, 90 }, { 255, 40, 0 } },
    /* BO_ORB_ICE      */ { 2, 0x10, 22, 44, 120, 3.5f, 0.0f, { 230, 250, 255 }, { 80, 170, 255 } },
    /* BO_ORB_CURSE    */ { 0, 0x10, 22, 44, 120, 3.5f, 0.0f, { 235, 170, 255 }, { 130, 0, 210 } },
    /* BO_ORB_LIGHT    */ { 3, 0x10, 24, 48, 150, 4.0f, 0.0f, { 255, 255, 235 }, { 255, 230, 110 } },
    /* BO_ORB_MUD      */ { 4, 0x08, 20, 40, 90, 3.0f, 0.0f, { 205, 170, 120 }, { 90, 60, 20 } },
};

// Enemy attack that can be blocked with a shield. It never takes damage itself.
static ColliderCylinderInit sBoOrbCylinderInit = {
    {
        COL_MATERIAL_NONE,
        AT_ON | AT_TYPE_ENEMY,
        AC_NONE,
        OC1_NONE,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK0,
        { 0xF7CFFFFF, 0x01, 0x10 },
        { 0x00000000, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 22, 44, -22, { 0, 0, 0 } },
};

void BoOrb_Init(Actor* actor, BoOrb* orb, PlayState* play) {
    BoOrbKindInfo* info;
    s16 kind = actor->world.rot.z;

    if ((kind < 0) || (kind >= BO_ORB_KIND_MAX)) {
        kind = BO_ORB_FIRE;
    }

    info = &sBoOrbKindInfo[kind];

    actor->world.rot.z = 0;
    actor->shape.rot.z = 0;
    actor->flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    actor->colChkInfo.health = 0;
    actor->gravity = 0.0f;
    actor->velocity = gZeroVec3f;
    actor->shape.shadowDraw = NULL;
    Actor_SetScale(actor, 0.01f);

    // Keep orbs out of the boss list: several bosses look themselves up by taking the first actor of their id there.
    Actor_ChangeCategory(play, &play->actorCtx, actor, ACTORCAT_MISC);

    orb->kind = kind;
    orb->life = orb->maxLife = info->life;
    orb->spin = (s16)Rand_Next();
    orb->gravity = 0.0f;
    orb->drag = 1.0f;
    orb->drawScale = info->drawScale;
    orb->growth = info->growth;
    orb->sizeScale = 1.0f;
    orb->dieOnSurface = BO_ORB_DIE_ON_ANY_SURFACE;

    Collider_InitAndSetCylinder(play, &orb->collider, actor, &sBoOrbCylinderInit);
    orb->collider.elem.atDmgInfo.effect = info->atEffect;
    orb->collider.elem.atDmgInfo.damage = info->damage;
}

void BoOrb_Destroy(Actor* actor, BoOrb* orb, PlayState* play) {
    Collider_DestroyCylinder(play, &orb->collider);
}

void BoOrb_Update(Actor* actor, BoOrb* orb, PlayState* play) {
    BoOrbKindInfo* info = &sBoOrbKindInfo[orb->kind];

    actor->velocity.x *= orb->drag;
    actor->velocity.z *= orb->drag;
    actor->velocity.y = (actor->velocity.y * orb->drag) + orb->gravity;

    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.y += actor->velocity.y;
    actor->world.pos.z += actor->velocity.z;
    actor->shape.rot.z += 0x1200;
    orb->spin += 0x1200;
    orb->drawScale += orb->growth;

    if (orb->life > 0) {
        orb->life--;
    }

    if ((orb->kind == BO_ORB_FIRE) || (orb->kind == BO_ORB_EMBER)) {
        // Real flames trailing the glow
        // Every other frame for fireballs, every third for embers: a breath can have dozens of these alive at once
        if ((orb->life % ((orb->kind == BO_ORB_FIRE) ? 2 : 3)) == 0) {
            Vec3f flamePos;
            Vec3f flameAccel = { 0.0f, 0.4f, 0.0f };

            flamePos.x = actor->world.pos.x + Rand_CenteredFloat(20.0f * orb->sizeScale);
            flamePos.y = actor->world.pos.y + Rand_CenteredFloat(20.0f * orb->sizeScale);
            flamePos.z = actor->world.pos.z + Rand_CenteredFloat(20.0f * orb->sizeScale);
            EffectSsKFire_Spawn(play, &flamePos, &gZeroVec3f, &flameAccel,
                                (s16)((Rand_ZeroFloat(10.0f) + 20.0f) * orb->sizeScale), 100);
        }
    }

    if (((orb->kind == BO_ORB_ICE) || (orb->kind == BO_ORB_CURSE) || (orb->kind == BO_ORB_LIGHT)) &&
        ((orb->life % 3) == 0)) {
        // A glittering trail in the orb's colors
        Color_RGBA8 sparklePrim = { 255, 255, 255, 255 };
        Color_RGBA8 sparkleEnv = { info->env.r, info->env.g, info->env.b, 0 };
        Vec3f sparklePos;

        sparklePos.x = actor->world.pos.x + Rand_CenteredFloat(24.0f * orb->sizeScale);
        sparklePos.y = actor->world.pos.y + Rand_CenteredFloat(24.0f * orb->sizeScale);
        sparklePos.z = actor->world.pos.z + Rand_CenteredFloat(24.0f * orb->sizeScale);
        EffectSsKirakira_SpawnSmall(play, &sparklePos, &gZeroVec3f, &gZeroVec3f, &sparklePrim, &sparkleEnv);
    }

    if (orb->kind == BO_ORB_FIRE) {
        Actor_PlaySfx(actor, NA_SE_EV_BURN_OUT - SFX_FLAG);
    } else if (orb->kind == BO_ORB_ELECTRIC) {
        Actor_PlaySfx(actor, NA_SE_EN_COMMON_THUNDER - SFX_FLAG);
    }

    if (orb->dieOnSurface == BO_ORB_DIE_ON_WALLS) {
        Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 20.0f, UPDBGCHECKINFO_FLAG_1);

        if (actor->bgCheckFlags & BGCHECKFLAG_WALL) {
            orb->life = 0;
        }
    } else if (orb->dieOnSurface != BO_ORB_DIE_NEVER) {
        Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 20.0f,
                                UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_4);

        if (actor->bgCheckFlags & (BGCHECKFLAG_GROUND | BGCHECKFLAG_WALL | BGCHECKFLAG_CEILING)) {
            orb->life = 0;
        }
    }

    orb->collider.dim.radius = info->radius * orb->sizeScale * ((orb->drawScale / info->drawScale + 1.0f) * 0.5f);
    orb->collider.dim.height = info->height * orb->sizeScale;
    orb->collider.dim.yShift = -orb->collider.dim.height / 2;
    Collider_UpdateCylinder(actor, &orb->collider);

    if (orb->collider.base.atFlags & AT_HIT) {
        // Hit Link, or was stopped by his shield
        orb->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        orb->life = 0;
    }

    if (orb->life == 0) {
        if (orb->kind == BO_ORB_WATER) {
            EffectSsGSplash_Spawn(play, &actor->world.pos, NULL, NULL, 0, (s16)(700.0f * orb->sizeScale));
            Actor_PlaySfx(actor, NA_SE_EV_BOMB_DROP_WATER);
        } else if (orb->kind == BO_ORB_ICE) {
            EffectSsIcePiece_SpawnBurst(play, &actor->world.pos, 0.6f * orb->sizeScale);
            Actor_PlaySfx(actor, NA_SE_EV_ICE_BROKEN);
        } else if ((orb->kind == BO_ORB_CURSE) || (orb->kind == BO_ORB_LIGHT)) {
            Color_RGBA8 puffPrim = { info->prim.r, info->prim.g, info->prim.b, 255 };
            Color_RGBA8 puffEnv = { info->env.r, info->env.g, info->env.b, 255 };

            EffectSsDeadDb_Spawn(play, &actor->world.pos, &gZeroVec3f, &gZeroVec3f, &puffPrim, &puffEnv,
                                 (s16)(60.0f * orb->sizeScale), 6, 9);
        } else if (orb->kind == BO_ORB_MUD) {
            func_800B1210(play, &actor->world.pos, &gZeroVec3f, &gZeroVec3f, (s16)(120.0f * orb->sizeScale), 20);
            Actor_PlaySfx(actor, NA_SE_EN_OCTAROCK_ROCK);
        } else if (orb->kind != BO_ORB_EMBER) {
            Vec3f flameAccel = { 0.0f, 0.5f, 0.0f };

            EffectSsKFire_Spawn(play, &actor->world.pos, &gZeroVec3f, &flameAccel, (s16)(40.0f * orb->sizeScale),
                                (orb->kind == BO_ORB_FIRE) ? 100 : 0);
        }
        Actor_Kill(actor);
        return;
    }

    CollisionCheck_SetAT(play, &play->colChkCtx, &orb->collider.base);
}

void BoOrb_Draw(Actor* actor, BoOrb* orb, PlayState* play) {
    BoOrbKindInfo* info = &sBoOrbKindInfo[orb->kind];
    f32 scale = orb->drawScale * orb->sizeScale;
    s16 alpha = 255;

    if (orb->life < 8) {
        alpha = orb->life * 32;
    }

    if ((orb->kind == BO_ORB_EMBER) && (orb->maxLife > 0)) {
        alpha = (alpha * orb->life) / orb->maxLife;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, info->prim.r, info->prim.g, info->prim.b, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, info->env.r, info->env.g, info->env.b, 128);
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    Matrix_Scale(scale, scale, 0.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_RotateZS(orb->spin, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);
    gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

Actor* BoOrb_Spawn(PlayState* play, s16 actorId, BoOrbKind kind, Vec3f* pos, Vec3f* vel) {
    Actor* orb = Actor_Spawn(&play->actorCtx, play, actorId, pos->x, pos->y, pos->z, 0, 0, kind, BO_ORB_PARAMS);

    if (orb != NULL) {
        orb->velocity = *vel;
        orb->world.rot.y = Math_Atan2S(vel->x, vel->z);
        orb->shape.rot.y = orb->world.rot.y;
    }

    return orb;
}

void BoOrb_AimVelocity(Vec3f* from, Vec3f* to, f32 speed, s16 yawOffset, Vec3f* outVel) {
    f32 dx = to->x - from->x;
    f32 dy = to->y - from->y;
    f32 dz = to->z - from->z;
    f32 dist = sqrtf(SQ(dx) + SQ(dy) + SQ(dz));
    f32 cosOff = Math_CosS(yawOffset);
    f32 sinOff = Math_SinS(yawOffset);

    if (dist < 1.0f) {
        dx = 0.0f;
        dy = 0.0f;
        dz = 1.0f;
        dist = 1.0f;
    }

    dx = (dx / dist) * speed;
    dy = (dy / dist) * speed;
    dz = (dz / dist) * speed;

    outVel->x = (dx * cosOff) + (dz * sinOff);
    outVel->y = dy;
    outVel->z = (dz * cosOff) - (dx * sinOff);
}

void BoOrb_ArcVelocity(Vec3f* from, Vec3f* to, f32 frames, f32 gravity, Vec3f* outVel) {
    if (frames < 1.0f) {
        frames = 1.0f;
    }

    outVel->x = (to->x - from->x) / frames;
    outVel->z = (to->z - from->z) / frames;
    // y(t) = vy * t + gravity * t * (t + 1) / 2, solved for vy at t = frames
    outVel->y = ((to->y - from->y) / frames) - (gravity * (frames + 1.0f) * 0.5f);
}

s32 BoMM_IsFierceDeity(PlayState* play) {
    return GET_PLAYER(play)->transformation == PLAYER_FORM_FIERCE_DEITY;
}

u8 BoMM_MajoraDamage(PlayState* play, u8 damage) {
    if ((damage != 0) && BO_FIERCE_DEITY_ENABLED() && BoMM_IsFierceDeity(play)) {
        damage = (damage + 1) / 2;
        if (damage == 0) {
            damage = 1;
        }
    }

    return damage;
}
