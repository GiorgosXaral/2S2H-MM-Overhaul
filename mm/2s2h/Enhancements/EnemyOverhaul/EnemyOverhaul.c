#include "EnemyOverhaul.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"

/**
 * Miniboss & Enemy Overhaul: shots, summons and helpers shared by every overhauled enemy. See EnemyOverhaul.h.
 */

s32 gEoSpawningAnyObject = 0;

/* ---------------------------------------------------------------------------------------------------------------
 * Shots
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_SHOT_POOL_SIZE 96

static EoShot sEoShots[EO_SHOT_POOL_SIZE];

void EoShot_Update(Actor* thisx, PlayState* play);
void EoShot_Draw(Actor* thisx, PlayState* play);
void EoShot_Destroy(Actor* thisx, PlayState* play);

EoShot* EoShot_Get(Actor* host) {
    s32 i;

    if (host == NULL) {
        return NULL;
    }

    for (i = 0; i < EO_SHOT_POOL_SIZE; i++) {
        if (sEoShots[i].actor == host) {
            return &sEoShots[i];
        }
    }
    return NULL;
}

s32 EoShot_CountOwned(Actor* owner, s32 mode) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_SHOT_POOL_SIZE; i++) {
        if ((sEoShots[i].actor != NULL) && (sEoShots[i].owner == owner) && (sEoShots[i].mode == mode)) {
            count++;
        }
    }
    return count;
}

/**
 * The host is an En_Item00 with "nothing" in it: always available, drawn by us, never counted as an enemy, and its
 * pickup code never runs because its update is replaced before its first frame.
 */
static EoShot* EoShot_Alloc(PlayState* play, Actor* owner, s32 mode, BoOrbKind kind, Vec3f* pos) {
    EoShot* shot = NULL;
    Actor* host;
    s32 i;

    if ((play == NULL) || (pos == NULL)) {
        return NULL;
    }

    for (i = 0; i < EO_SHOT_POOL_SIZE; i++) {
        if (sEoShots[i].actor == NULL) {
            shot = &sEoShots[i];
            break;
        }
    }

    if (shot == NULL) {
        return NULL;
    }

    host = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_ITEM00, pos->x, pos->y, pos->z, 0, 0, 0, ITEM00_NOTHING);
    if (host == NULL) {
        return NULL;
    }

    memset(shot, 0, sizeof(EoShot));
    shot->actor = host;
    shot->owner = owner;
    shot->mode = mode;
    shot->dListScale = 1.0f;

    host->update = EoShot_Update;
    host->draw = EoShot_Draw;
    host->destroy = EoShot_Destroy;
    host->room = -1; // follows Link through room changes instead of being unloaded mid-flight
    host->flags |= ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED;
    host->flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
    host->world.rot.z = kind;
    BoOrb_Init(host, &shot->orb, play);
    host->world.pos = *pos;
    host->prevPos = *pos;
    host->home.pos = *pos;
    return shot;
}

EoShot* EoShot_Orb(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, Vec3f* vel) {
    EoShot* shot = EoShot_Alloc(play, owner, EO_SHOT_ORB, kind, pos);

    if (shot != NULL) {
        shot->actor->velocity = *vel;
        shot->actor->world.rot.y = shot->actor->shape.rot.y = Math_Atan2S(vel->x, vel->z);
    }
    return shot;
}

static void Eo_PlayerAimPoint(PlayState* play, Vec3f* out) {
    Player* player = GET_PLAYER(play);

    *out = player->actor.world.pos;
    out->y += 30.0f;
}

EoShot* EoShot_OrbAimed(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s16 yawOffset) {
    Vec3f target;
    Vec3f vel;

    Eo_PlayerAimPoint(play, &target);
    BoOrb_AimVelocity(pos, &target, speed, yawOffset, &vel);
    return EoShot_Orb(play, owner, kind, pos, &vel);
}

void EoShot_OrbFan(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s32 count, s16 spread) {
    s32 i;

    for (i = 0; i < count; i++) {
        EoShot_OrbAimed(play, owner, kind, pos, speed, (s16)((i * 2 - (count - 1)) * spread / 2));
    }
}

void EoShot_OrbRing(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, f32 speed, s32 count, s16 baseYaw) {
    Vec3f vel;
    s32 i;

    for (i = 0; i < count; i++) {
        s16 yaw = baseYaw + (s16)(i * (0x10000 / count));

        vel.x = Math_SinS(yaw) * speed;
        vel.y = 0.0f;
        vel.z = Math_CosS(yaw) * speed;
        EoShot_Orb(play, owner, kind, pos, &vel);
    }
}

EoShot* EoShot_OrbLob(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* pos, Vec3f* to, f32 frames) {
    Vec3f vel;
    EoShot* shot;

    BoOrb_ArcVelocity(pos, to, frames, -1.0f, &vel);
    shot = EoShot_Orb(play, owner, kind, pos, &vel);
    if (shot != NULL) {
        shot->orb.gravity = -1.0f;
        shot->orb.life = shot->orb.maxLife = (s16)frames + 30;
    }
    return shot;
}

static void EoShot_SetHitbox(EoShot* shot, s16 radius, s16 height, u8 damage, u8 effect) {
    shot->orb.collider.dim.radius = radius;
    shot->orb.collider.dim.height = height;
    shot->orb.collider.dim.yShift = 0;
    shot->orb.collider.elem.atDmgInfo.damage = damage;
    shot->orb.collider.elem.atDmgInfo.effect = effect;
}

EoShot* EoShot_GroundWave(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, s16 yaw, f32 speed, s16 life,
                          u8 damage) {
    EoShot* shot = EoShot_Alloc(play, owner, EO_SHOT_GROUND_WAVE, BO_ORB_EMBER, pos);

    if (shot != NULL) {
        shot->look = look;
        shot->damage = damage;
        shot->speed = speed;
        shot->timer = life;
        shot->actor->world.rot.y = yaw;
        shot->actor->velocity.x = Math_SinS(yaw) * speed;
        shot->actor->velocity.z = Math_CosS(yaw) * speed;
        EoShot_SetHitbox(shot, 28, 50, damage, (look == EO_LOOK_FIRE) ? 1 : ((look == EO_LOOK_WATER) ? 4 : 0));
    }
    return shot;
}

void EoShot_GroundWaveFan(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, s32 count, s16 spread, f32 speed,
                          s16 life, u8 damage) {
    Player* player = GET_PLAYER(play);
    s16 yaw = Math_Vec3f_Yaw(pos, &player->actor.world.pos);
    s32 i;

    for (i = 0; i < count; i++) {
        EoShot_GroundWave(play, owner, look, pos, yaw + (s16)((i * 2 - (count - 1)) * spread / 2), speed, life,
                          damage);
    }
}

EoShot* EoShot_Ring(PlayState* play, Actor* owner, EoLook look, Vec3f* pos, f32 radiusMax, f32 radiusStep, u8 damage) {
    EoShot* shot = EoShot_Alloc(play, owner, EO_SHOT_RING, BO_ORB_EMBER, pos);

    if (shot != NULL) {
        shot->look = look;
        shot->damage = damage;
        shot->radius = 20.0f;
        shot->radiusMax = radiusMax;
        shot->radiusStep = radiusStep;
        shot->actor->floorHeight = pos->y;
    }
    return shot;
}

EoShot* EoShot_Boomerang(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* pos, s16 yaw, f32 speed,
                         u8 damage) {
    EoShot* shot = EoShot_Alloc(play, owner, EO_SHOT_BOOMERANG, BO_ORB_EMBER, pos);

    if (shot != NULL) {
        shot->dList = dList;
        shot->dListScale = scale;
        shot->speed = speed;
        shot->damage = damage;
        shot->spinStep = 0x2400;
        shot->actor->world.rot.y = yaw;
        EoShot_SetHitbox(shot, 36, 36, damage, 0);
        shot->orb.collider.dim.yShift = -18;
    }
    return shot;
}

EoShot* EoShot_Falling(PlayState* play, Actor* owner, Gfx* dList, f32 scale, Vec3f* spot, f32 height, u8 damage) {
    Vec3f pos = *spot;
    EoShot* shot;

    pos.y += height;
    shot = EoShot_Alloc(play, owner, EO_SHOT_FALLING, BO_ORB_EMBER, &pos);
    if (shot != NULL) {
        shot->dList = dList;
        shot->dListScale = scale;
        shot->damage = damage;
        shot->target = *spot;
        shot->spinStep = (s16)Rand_CenteredFloat(0x1800);
        shot->actor->floorHeight = spot->y;
    }
    return shot;
}

EoShot* EoShot_FallingOrb(PlayState* play, Actor* owner, BoOrbKind kind, Vec3f* spot, f32 height, u8 damage) {
    Vec3f pos = *spot;
    EoShot* shot;

    pos.y += height;
    shot = EoShot_Alloc(play, owner, EO_SHOT_FALLING, kind, &pos);
    if (shot != NULL) {
        shot->dList = NULL; // drawn as the orb
        shot->damage = damage;
        shot->target = *spot;
        shot->actor->floorHeight = spot->y;
    }
    return shot;
}

static void EoShot_LookPuff(PlayState* play, EoShot* shot, Vec3f* pos, f32 scale) {
    static Color_RGBA8 sSnowPrim = { 250, 250, 255, 255 };
    static Color_RGBA8 sSnowEnv = { 180, 200, 230, 255 };
    static Color_RGBA8 sDarkPrim = { 200, 120, 255, 255 };
    static Color_RGBA8 sDarkEnv = { 60, 0, 90, 255 };
    Vec3f accel = { 0.0f, 0.4f, 0.0f };

    switch (shot->look) {
        case EO_LOOK_FIRE:
            EffectSsKFire_Spawn(play, pos, &gZeroVec3f, &accel, (s16)(25.0f * scale), 100);
            break;

        case EO_LOOK_SNOW:
            func_800B0EB0(play, pos, &gZeroVec3f, &accel, &sSnowPrim, &sSnowEnv, (s16)(140.0f * scale), 20, 12);
            break;

        case EO_LOOK_WATER:
            EffectSsGSplash_Spawn(play, pos, NULL, NULL, 1, (s16)(300.0f * scale));
            break;

        case EO_LOOK_DARK:
            EffectSsDeadDb_Spawn(play, pos, &gZeroVec3f, &accel, &sDarkPrim, &sDarkEnv, (s16)(60.0f * scale), 4, 10);
            break;

        default:
            func_800B1210(play, pos, &gZeroVec3f, &accel, (s16)(160.0f * scale), 20);
            break;
    }
}

static void EoShot_UpdateOrb(EoShot* shot, PlayState* play) {
    Actor* actor = shot->actor;

    if ((shot->homing != 0) && Eo_CanAttack(play)) {
        Vec3f target;
        f32 speedXZ = sqrtf(SQ(actor->velocity.x) + SQ(actor->velocity.z));
        s16 yaw = Math_Atan2S(actor->velocity.x, actor->velocity.z);

        Eo_PlayerAimPoint(play, &target);
        Math_ScaledStepToS(&yaw, Math_Vec3f_Yaw(&actor->world.pos, &target), shot->homing);
        actor->velocity.x = Math_SinS(yaw) * speedXZ;
        actor->velocity.z = Math_CosS(yaw) * speedXZ;
    }

    BoOrb_Update(actor, &shot->orb, play);
}

static void EoShot_UpdateGroundWave(EoShot* shot, PlayState* play) {
    Actor* actor = shot->actor;
    f32 prevFloor = actor->floorHeight;

    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.z += actor->velocity.z;
    actor->world.pos.y += 30.0f;
    Actor_UpdateBgCheckInfo(play, actor, 20.0f, 20.0f, 40.0f, UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_4);
    actor->world.pos.y = actor->floorHeight;

    // A wall, a gap in the floor or a long drop puts it out
    if ((actor->bgCheckFlags & BGCHECKFLAG_WALL) || (actor->floorHeight <= BGCHECK_Y_MIN + 1.0f) ||
        ((prevFloor != 0.0f) && (fabsf(actor->floorHeight - prevFloor) > 60.0f))) {
        shot->timer = 0;
    }

    if ((shot->timer % 2) == 0) {
        Vec3f puffPos = actor->world.pos;

        puffPos.x += Rand_CenteredFloat(20.0f);
        puffPos.z += Rand_CenteredFloat(20.0f);
        EoShot_LookPuff(play, shot, &puffPos, 1.0f);
    }

    if (shot->look == EO_LOOK_FIRE) {
        Actor_PlaySfx(actor, NA_SE_EV_BURN_OUT - SFX_FLAG);
    }

    Collider_UpdateCylinder(actor, &shot->orb.collider);
    if (shot->orb.collider.base.atFlags & AT_HIT) {
        shot->orb.collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        shot->timer = 0;
    }

    if (shot->timer <= 0) {
        EoShot_LookPuff(play, shot, &actor->world.pos, 1.6f);
        Actor_Kill(actor);
        return;
    }
    shot->timer--;
    CollisionCheck_SetAT(play, &play->colChkCtx, &shot->orb.collider.base);
}

static void EoShot_UpdateRing(EoShot* shot, PlayState* play) {
    Actor* actor = shot->actor;
    Player* player = GET_PLAYER(play);
    f32 dist;

    shot->radius += shot->radiusStep;
    if (shot->radius >= shot->radiusMax) {
        Actor_Kill(actor);
        return;
    }

    if ((shot->timer % 2) == 0) {
        s32 count = CLAMP((s32)(shot->radius / 45.0f), 8, 28);
        s16 base = (s16)Rand_Next();
        s32 i;

        for (i = 0; i < count; i++) {
            s16 angle = base + (s16)(i * (0x10000 / count));
            Vec3f pos;

            pos.x = actor->world.pos.x + (Math_SinS(angle) * shot->radius);
            pos.y = actor->floorHeight + 2.0f;
            pos.z = actor->world.pos.z + (Math_CosS(angle) * shot->radius);
            EoShot_LookPuff(play, shot, &pos, 0.8f);
        }
    }
    shot->timer++;

    // One hit per ring, where the ring passes and only while Link stands on the floor
    if (shot->damage != 0) {
        dist = sqrtf(SQ(player->actor.world.pos.x - actor->world.pos.x) +
                     SQ(player->actor.world.pos.z - actor->world.pos.z));

        if ((fabsf(dist - shot->radius) < (shot->radiusStep + 30.0f)) &&
            (fabsf(player->actor.world.pos.y - actor->floorHeight) < 45.0f) &&
            Eo_HurtPlayer(play, (shot->owner != NULL) ? shot->owner : actor, shot->damage,
                          Math_Vec3f_Yaw(&actor->world.pos, &player->actor.world.pos), 9.0f, true, false)) {
            shot->damage = 0;
        }
    }
}

static void EoShot_UpdateBoomerang(EoShot* shot, PlayState* play) {
    Actor* actor = shot->actor;

    shot->timer++;
    shot->spin += shot->spinStep;
    Actor_PlaySfx(actor, NA_SE_IT_BOOMERANG_FLY - SFX_FLAG);

    if ((shot->timer < 34) && (shot->owner != NULL)) {
        // Out: slowing down, curving a little towards Link
        Player* player = GET_PLAYER(play);

        Math_ScaledStepToS(&actor->world.rot.y, Math_Vec3f_Yaw(&actor->world.pos, &player->actor.world.pos), 0x180);
        shot->speed *= 0.95f;
        actor->velocity.x = Math_SinS(actor->world.rot.y) * shot->speed;
        actor->velocity.z = Math_CosS(actor->world.rot.y) * shot->speed;
        actor->velocity.y = 0.0f;
    } else if (shot->owner != NULL) {
        // Back to its owner's hand
        Vec3f home = shot->owner->focus.pos;
        Vec3f vel;
        f32 dist = Math_Vec3f_DistXYZ(&actor->world.pos, &home);

        if ((dist < 60.0f) || (shot->timer > 200)) {
            Actor_Kill(actor);
            return;
        }
        Math_StepToF(&shot->speed, 22.0f, 1.2f);
        BoOrb_AimVelocity(&actor->world.pos, &home, shot->speed, 0, &vel);
        actor->velocity = vel;
    } else {
        Actor_Kill(actor);
        return;
    }

    actor->world.pos.x += actor->velocity.x;
    actor->world.pos.y += actor->velocity.y;
    actor->world.pos.z += actor->velocity.z;

    if ((shot->timer % 3) == 0) {
        Vec3f dustPos = actor->world.pos;

        func_800B1210(play, &dustPos, &gZeroVec3f, &gZeroVec3f, 60, 10);
    }

    Collider_UpdateCylinder(actor, &shot->orb.collider);
    if (shot->orb.collider.base.atFlags & (AT_HIT | AT_BOUNCED)) {
        shot->orb.collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        shot->timer = MAX(shot->timer, 34); // bounced off Link or his shield: it heads home
        shot->radius = 12.0f;               // no second hit for a few frames
    }
    if (shot->radius > 0.0f) {
        shot->radius -= 1.0f;
    } else {
        CollisionCheck_SetAT(play, &play->colChkCtx, &shot->orb.collider.base);
    }
}

static void EoShot_UpdateFalling(EoShot* shot, PlayState* play) {
    Actor* actor = shot->actor;
    Player* player = GET_PLAYER(play);

    shot->spin += shot->spinStep;
    actor->velocity.y -= 1.6f;
    actor->world.pos.y += actor->velocity.y;

    // The spot it will land on is marked with dust all the way down
    if ((shot->timer % 5) == 0) {
        Vec3f mark = shot->target;

        mark.x += Rand_CenteredFloat(30.0f);
        mark.z += Rand_CenteredFloat(30.0f);
        func_800B1210(play, &mark, &gZeroVec3f, &gZeroVec3f, 90, 8);
    }
    shot->timer++;

    if (actor->world.pos.y <= shot->target.y) {
        f32 dist = sqrtf(SQ(player->actor.world.pos.x - shot->target.x) + SQ(player->actor.world.pos.z - shot->target.z));
        s32 i;

        actor->world.pos.y = shot->target.y;
        for (i = 0; i < 4; i++) {
            Vec3f puffPos = shot->target;

            puffPos.x += Rand_CenteredFloat(60.0f);
            puffPos.z += Rand_CenteredFloat(60.0f);
            func_800B1210(play, &puffPos, &gZeroVec3f, &gZeroVec3f, 200, 25);
        }
        if (shot->dList == NULL) {
            Actor_PlaySfx(actor, NA_SE_EN_FOLLOWERS_BEAM);
        } else {
            Actor_PlaySfx(actor, NA_SE_EV_ROCK_BROKEN);
        }

        if ((dist < 70.0f) && (fabsf(player->actor.world.pos.y - shot->target.y) < 80.0f)) {
            Eo_HurtPlayer(play, (shot->owner != NULL) ? shot->owner : actor, shot->damage,
                          Math_Vec3f_Yaw(&shot->target, &player->actor.world.pos), 6.0f, false, false);
        }
        Actor_Kill(actor);
    }
}

void EoShot_Update(Actor* thisx, PlayState* play) {
    EoShot* shot = EoShot_Get(thisx);

    if (shot == NULL) {
        Actor_Kill(thisx);
        return;
    }

    // Shots vanish in cutscenes (a miniboss dying, a door opening, ...)
    if (Play_InCsMode(play) && (shot->mode != EO_SHOT_ORB)) {
        Actor_Kill(thisx);
        return;
    }

    switch (shot->mode) {
        case EO_SHOT_ORB:
            EoShot_UpdateOrb(shot, play);
            break;
        case EO_SHOT_GROUND_WAVE:
            EoShot_UpdateGroundWave(shot, play);
            break;
        case EO_SHOT_RING:
            EoShot_UpdateRing(shot, play);
            break;
        case EO_SHOT_BOOMERANG:
            EoShot_UpdateBoomerang(shot, play);
            break;
        case EO_SHOT_FALLING:
            EoShot_UpdateFalling(shot, play);
            break;
        default:
            Actor_Kill(thisx);
            break;
    }
}

void EoShot_Draw(Actor* thisx, PlayState* play) {
    EoShot* shot = EoShot_Get(thisx);

    if (shot == NULL) {
        return;
    }

    if ((shot->mode == EO_SHOT_ORB) || ((shot->mode == EO_SHOT_FALLING) && (shot->dList == NULL))) {
        BoOrb_Draw(thisx, &shot->orb, play);
    } else if (((shot->mode == EO_SHOT_BOOMERANG) || (shot->mode == EO_SHOT_FALLING)) && (shot->dList != NULL)) {
        OPEN_DISPS(play->state.gfxCtx);

        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        Matrix_Translate(thisx->world.pos.x, thisx->world.pos.y, thisx->world.pos.z, MTXMODE_NEW);
        if (shot->mode == EO_SHOT_BOOMERANG) {
            Matrix_RotateYS(shot->spin, MTXMODE_APPLY);
            Matrix_RotateZS(0x4000, MTXMODE_APPLY);
        } else {
            Matrix_RotateYS(shot->spin, MTXMODE_APPLY);
            Matrix_RotateXS(shot->spin, MTXMODE_APPLY);
        }
        Matrix_Scale(shot->dListScale, shot->dListScale, shot->dListScale, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, shot->dList);

        CLOSE_DISPS(play->state.gfxCtx);
    }
}

void EoShot_Destroy(Actor* thisx, PlayState* play) {
    EoShot* shot = EoShot_Get(thisx);

    Collider_DestroyCylinder(play, &((EnItem00*)thisx)->collider);
    if (shot != NULL) {
        BoOrb_Destroy(thisx, &shot->orb, play);
        shot->actor = NULL;
        shot->owner = NULL;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Summons
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_SUMMON_MAX 32

typedef struct EoSummonEntry {
    Actor* actor;
    Actor* owner;
} EoSummonEntry;

static EoSummonEntry sEoSummons[EO_SUMMON_MAX];

Actor* Eo_SpawnAnyObject(PlayState* play, s16 actorId, f32 x, f32 y, f32 z, s16 rx, s16 ry, s16 rz, s32 params) {
    Actor* actor;

    gEoSpawningAnyObject++;
    actor = Actor_Spawn(&play->actorCtx, play, actorId, x, y, z, rx, ry, rz, params);
    gEoSpawningAnyObject--;
    return actor;
}

Actor* EoSummon_Spawn(PlayState* play, Actor* owner, s16 actorId, Vec3f* pos, s16 rotY, s32 params) {
    Actor* actor;
    s32 i;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if (sEoSummons[i].actor == NULL) {
            break;
        }
    }
    if (i == EO_SUMMON_MAX) {
        return NULL;
    }

    actor = Eo_SpawnAnyObject(play, actorId, pos->x, pos->y, pos->z, 0, rotY, 0, params);
    if (actor != NULL) {
        sEoSummons[i].actor = actor;
        sEoSummons[i].owner = owner;
        Eo_Puff(play, pos, 1.5f);
    }
    return actor;
}

s32 EoSummon_Count(Actor* owner) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == owner) &&
            (sEoSummons[i].actor->update != NULL)) {
            count++;
        }
    }
    return count;
}

void EoSummon_KillAll(Actor* owner, PlayState* play) {
    s32 i;

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == owner)) {
            if (sEoSummons[i].actor->update != NULL) {
                if (play != NULL) {
                    Eo_Puff(play, &sEoSummons[i].actor->world.pos, 2.0f);
                }
                Actor_Kill(sEoSummons[i].actor);
            }
            sEoSummons[i].owner = NULL;
        }
    }
}

void EnemyOverhaul_OnActorDestroy(Actor* actor) {
    s32 i;

    EoEncounters_OnActorDestroy(actor);

    for (i = 0; i < EO_SHOT_POOL_SIZE; i++) {
        if (sEoShots[i].owner == actor) {
            sEoShots[i].owner = NULL;
        }
    }

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if (sEoSummons[i].actor == actor) {
            sEoSummons[i].actor = NULL;
            sEoSummons[i].owner = NULL;
        }
    }

    // Anything this actor summoned goes with it
    EoSummon_KillAll(actor, NULL);

    for (i = 0; i < EO_SUMMON_MAX; i++) {
        if ((sEoSummons[i].actor != NULL) && (sEoSummons[i].owner == NULL)) {
            sEoSummons[i].actor = NULL;
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------------------------------------------------- */

s32 Eo_CanAttack(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return !Play_InCsMode(play) && !Player_InBlockingCsMode(play, player) &&
           (gSaveContext.save.saveInfo.playerData.health > 0);
}

s32 Eo_PlayerOnGround(PlayState* play) {
    Player* player = GET_PLAYER(play);

    return (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) && !(player->stateFlags1 & PLAYER_STATE1_800000);
}

s32 Eo_HurtPlayer(PlayState* play, Actor* source, u8 damage, s16 yaw, f32 knockback, s32 groundOnly, s32 shock) {
    Player* player = GET_PLAYER(play);

    if ((player->invincibilityTimer != 0) || !Eo_CanAttack(play) || (player->stateFlags1 & PLAYER_STATE1_800000)) {
        return false;
    }
    if (groundOnly && !Eo_PlayerOnGround(play)) {
        return false;
    }

    func_800B8D10(play, source, knockback, yaw, knockback * 0.6f, shock ? 4 : 3, damage);
    return true;
}

s32 Eo_FindFloor(PlayState* play, Vec3f* pos, f32 maxDrop, f32* outY) {
    CollisionPoly* poly;
    Vec3f probe = *pos;
    f32 floorY;

    probe.y += 80.0f;
    floorY = BgCheck_EntityRaycastFloor1(&play->colCtx, &poly, &probe);
    if ((floorY <= BGCHECK_Y_MIN + 1.0f) || ((pos->y - floorY) > maxDrop)) {
        return false;
    }
    *outY = floorY;
    return true;
}

s32 Eo_PosBehindPlayer(PlayState* play, f32 dist, Vec3f* outPos) {
    Player* player = GET_PLAYER(play);
    Vec3f from = player->actor.world.pos;
    Vec3f hitPos;
    CollisionPoly* poly;
    s32 bgId;
    f32 floorY;

    outPos->x = player->actor.world.pos.x - (Math_SinS(player->actor.shape.rot.y) * dist);
    outPos->y = player->actor.world.pos.y + 30.0f;
    outPos->z = player->actor.world.pos.z - (Math_CosS(player->actor.shape.rot.y) * dist);
    from.y += 30.0f;

    if (!BgCheck_EntityLineTest1(&play->colCtx, &from, outPos, &hitPos, &poly, true, false, false, true, &bgId) &&
        Eo_FindFloor(play, outPos, 150.0f, &floorY)) {
        outPos->y = floorY;
        return true;
    }

    *outPos = player->actor.world.pos;
    return false;
}

void Eo_Puff(PlayState* play, Vec3f* pos, f32 scale) {
    static Color_RGBA8 sPuffPrim = { 250, 250, 250, 255 };
    static Color_RGBA8 sPuffEnv = { 150, 150, 150, 255 };
    Vec3f puffPos = *pos;

    puffPos.y += 20.0f * scale;
    EffectSsDeadDb_Spawn(play, &puffPos, &gZeroVec3f, &gZeroVec3f, &sPuffPrim, &sPuffEnv, (s16)(70.0f * scale), 6, 10);
    SoundSource_PlaySfxAtFixedWorldPos(play, pos, 20, NA_SE_EN_EXTINCT);
}
