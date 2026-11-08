/*
 * File: z_en_dragon.c
 * Overlay: ovl_En_Dragon
 * Description: Deep Python
 */

#include "z_en_dragon.h"
#include "2s2h/Enhancements/EnemyOverhaul/EnemyOverhaul.h"
#include "overlays/actors/ovl_En_Ot/z_en_ot.h"

#define FLAGS                                                                                 \
    (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE | ACTOR_FLAG_UPDATE_CULLING_DISABLED | \
     ACTOR_FLAG_DRAW_CULLING_DISABLED)

void EnDragon_Init(Actor* thisx, PlayState* play);
void EnDragon_Destroy(Actor* thisx, PlayState* play);
void EnDragon_Update(Actor* thisx, PlayState* play);
void EnDragon_Draw(Actor* thisx, PlayState* play);
void EnDragon_Reset(void);

void EnDragon_SetupRetreatOrIdle(EnDragon* this);
void EnDragon_RetreatOrIdle(EnDragon* this, PlayState* play);
void EnDragon_SetupExtend(EnDragon* this);
void EnDragon_Extend(EnDragon* this, PlayState* play);
void EnDragon_Grab(EnDragon* this, PlayState* play);
void EnDragon_SetupAttack(EnDragon* this);
void EnDragon_Attack(EnDragon* this, PlayState* play);
void EnDragon_Dead(EnDragon* this, PlayState* play);
void EnDragon_EoSwim(EnDragon* this, PlayState* play);
void EnDragon_TransformLimbDraw(PlayState* play, s32 limbIndex, Actor* thisx);

/**
 * Deep Python Overhaul (Enhancements > Difficulty Options > Miniboss & Enemy Overhaul > Deep Pythons)
 *
 * Every python: 8 health instead of 4, spits volleys of water at Link, and the last one alive becomes a miniboss
 * (miniboss music, 12 health, bigger volleys). The seahorse still appears once all eight are beaten.
 *
 * "Free swimming" on: once out of its burrow, a python leaves it and swims loops in front of its den, spitting water
 *   and lunging at Link, and heads home when hurt, when Link swims away, or after a while. Its tail is tapered and its
 *   body ripples as it swims. Grabs still drag Link back to the den for the vanilla bite.
 * "Free swimming" off (burrow hunters): pythons stay in their burrows but snap forward to bite Link without having to
 *   grab him first.
 */
typedef enum EnDragonEoMode {
    /* 0 */ EN_DRAGON_EO_NONE,
    /* 1 */ EN_DRAGON_EO_SWIM,
    /* 2 */ EN_DRAGON_EO_RETURN
} EnDragonEoMode;

typedef enum {
    /* 0 */ DEEP_PYTHON_EXTEND_STATE_NOT_FULLY_EXTENDED,
    /* 1 */ DEEP_PYTHON_EXTEND_STATE_FULLY_EXTENDED,
    /* 2 */ DEEP_PYTHON_EXTEND_STATE_REPEAT_LARGE_SWAY,
    /* 3 */ DEEP_PYTHON_EXTEND_STATE_REPEAT_SMALL_SWAY
} DeepPythonExtendState;

typedef enum {
    /* 0 */ DEEP_PYTHON_GRAB_STATE_START,
    /* 1 */ DEEP_PYTHON_GRAB_STATE_GRABBED
} DeepPythonGrabState;

typedef enum {
    /* 0 */ DEEP_PYTHON_ATTACK_STATE_START,
    /* 1 */ DEEP_PYTHON_ATTACK_STATE_RELEASED
} DeepPythonAttackState;

static s32 sNumPythonsDead = 0;

ActorProfile En_Dragon_Profile = {
    /**/ ACTOR_EN_DRAGON,
    /**/ ACTORCAT_ENEMY,
    /**/ FLAGS,
    /**/ OBJECT_UTUBO,
    /**/ sizeof(EnDragon),
    /**/ EnDragon_Init,
    /**/ EnDragon_Destroy,
    /**/ EnDragon_Update,
    /**/ EnDragon_Draw,
    /**/ EnDragon_Reset,
};

typedef enum {
    /* 0x0 */ DEEP_PYTHON_DMGEFF_NONE,
    /* 0xF */ DEEP_PYTHON_DMGEFF_DAMAGE = 0xF
} DeepPythonDamageEffect;

static DamageTable sDamageTable = {
    /* Deku Nut       */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Deku Stick     */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Horse trample  */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Explosives     */ DMG_ENTRY(1, DEEP_PYTHON_DMGEFF_DAMAGE),
    /* Zora boomerang */ DMG_ENTRY(1, DEEP_PYTHON_DMGEFF_DAMAGE),
    /* Normal arrow   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* UNK_DMG_0x06   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Hookshot       */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Goron punch    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Sword          */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Goron pound    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Fire arrow     */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Ice arrow      */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Light arrow    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Goron spikes   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Deku spin      */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Deku bubble    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Deku launch    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* UNK_DMG_0x12   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Zora barrier   */ DMG_ENTRY(1, DEEP_PYTHON_DMGEFF_DAMAGE),
    /* Normal shield  */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Light ray      */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Thrown object  */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Zora punch     */ DMG_ENTRY(1, DEEP_PYTHON_DMGEFF_DAMAGE),
    /* Spin attack    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Sword beam     */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Normal Roll    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* UNK_DMG_0x1B   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* UNK_DMG_0x1C   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Unblockable    */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* UNK_DMG_0x1E   */ DMG_ENTRY(0, DEEP_PYTHON_DMGEFF_NONE),
    /* Powder Keg     */ DMG_ENTRY(1, DEEP_PYTHON_DMGEFF_DAMAGE),
};

static ColliderJntSphElementInit sJntSphElementsInit[8] = {
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_HEAD, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_COLLAR, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_COLLAR, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_BODY_SEGMENT_1, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_BODY_SEGMENT_1, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_BODY_SEGMENT_2, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_BODY_SEGMENT_2, { { 0, 0, 0 }, 0 }, 1 },
    },
    {
        {
            ELEM_MATERIAL_UNK1,
            { 0xF7CFFFFF, 0x00, 0x00 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_NONE | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { DEEP_PYTHON_LIMB_BODY_SEGMENT_2, { { 0, 0, 0 }, 0 }, 1 },
    },
};

static ColliderJntSphInit sJntSphInit = {
    {
        COL_MATERIAL_HIT6,
        AT_NONE,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_1,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sJntSphElementsInit),
    sJntSphElementsInit,
};

void EnDragon_Init(Actor* thisx, PlayState* play) {
    EnDragon* this = (EnDragon*)thisx;

    SkelAnime_InitFlex(play, &this->skelAnime, &gDeepPythonSkel, &gDeepPythonSmallSideSwayAnim, this->jointTable,
                       this->morphTable, DEEP_PYTHON_LIMB_MAX);

    this->actor.colChkInfo.health = EO_PYTHON_ON() ? 8 : 4; // Deep Python Overhaul: 4 -> 8
    this->actor.colChkInfo.damageTable = &sDamageTable;
    this->actor.attentionRangeType = ATTENTION_RANGE_10;
    Collider_InitAndSetJntSph(play, &this->collider, &this->actor, &sJntSphInit, this->colliderElements);

    this->collider.elements[0].dim.scale = this->collider.elements[1].dim.scale = this->collider.elements[2].dim.scale =
        this->collider.elements[3].dim.scale = this->collider.elements[4].dim.scale =
            this->collider.elements[5].dim.scale = this->collider.elements[6].dim.scale =
                this->collider.elements[7].dim.scale = 1.0f;

    this->collider.elements[0].dim.modelSphere.radius = 150;
    this->collider.elements[0].dim.modelSphere.center.x = 420;
    this->collider.elements[1].dim.modelSphere.radius = 160;
    this->collider.elements[1].dim.modelSphere.center.x = 630;
    this->collider.elements[2].dim.modelSphere.radius = 130;
    this->collider.elements[2].dim.modelSphere.center.x = 630;
    this->collider.elements[3].dim.modelSphere.radius = 170;
    this->collider.elements[3].dim.modelSphere.center.x = 920;
    this->collider.elements[4].dim.modelSphere.radius = 150;
    this->collider.elements[4].dim.modelSphere.center.x = 530;
    this->collider.elements[5].dim.modelSphere.radius = 140;
    this->collider.elements[5].dim.modelSphere.center.x = 730;
    this->collider.elements[6].dim.modelSphere.radius = 120;
    this->collider.elements[6].dim.modelSphere.center.x = 430;
    this->collider.elements[7].dim.modelSphere.radius = 110;
    this->collider.elements[7].dim.modelSphere.center.x = 160;

    this->pythonIndex = EN_DRAGON_GET_PYTHON_INDEX(&this->actor);
    this->actor.colChkInfo.mass = MASS_IMMOVABLE;
    this->action = DEEP_PYTHON_ACTION_IDLE;
    this->actor.hintId = TATL_HINT_ID_DEEP_PYTHON;
    this->scale = 0.5f;
    this->actor.flags &= ~ACTOR_FLAG_LOCK_ON_DISABLED;

    EnDragon_SetupRetreatOrIdle(this);
}

void EnDragon_Destroy(Actor* thisx, PlayState* play) {
    EnDragon* this = (EnDragon*)thisx;

    Collider_DestroyJntSph(play, &this->collider);
}

typedef enum {
    /* 0 */ DEEP_PYTHON_ANIM_SMALL_SIDE_SWAY,
    /* 1 */ DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY,
    /* 2 */ DEEP_PYTHON_ANIM_VERTICAL_SWAY,
    /* 3 */ DEEP_PYTHON_ANIM_IDLE,
    /* 4 */ DEEP_PYTHON_ANIM_MAX
} DeepPythonAnimation;

static AnimationHeader* sAnimations[DEEP_PYTHON_ANIM_MAX] = {
    &gDeepPythonSmallSideSwayAnim, // DEEP_PYTHON_ANIM_SMALL_SIDE_SWAY
    &gDeepPythonLargeSideSwayAnim, // DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY
    &gDeepPythonVerticalSwayAnim,  // DEEP_PYTHON_ANIM_VERTICAL_SWAY
    &gDeepPythonSmallSideSwayAnim, // DEEP_PYTHON_ANIM_IDLE
};

static u8 sAnimationModes[DEEP_PYTHON_ANIM_MAX] = {
    ANIMMODE_LOOP, // DEEP_PYTHON_ANIM_SMALL_SIDE_SWAY
    ANIMMODE_LOOP, // DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY
    ANIMMODE_ONCE, // DEEP_PYTHON_ANIM_VERTICAL_SWAY
    ANIMMODE_ONCE, // DEEP_PYTHON_ANIM_IDLE
};

void EnDragon_ChangeAnim(EnDragon* this, s32 animIndex) {
    f32 startFrame;

    this->animIndex = animIndex;
    this->animEndFrame = Animation_GetLastFrame(sAnimations[animIndex]);
    startFrame = 0.0f;
    if (this->animIndex == DEEP_PYTHON_ANIM_IDLE) {
        startFrame = this->animEndFrame;
    }

    Animation_Change(&this->skelAnime, sAnimations[animIndex], 1.0f, startFrame, this->animEndFrame,
                     sAnimationModes[this->animIndex], -4.0f);
}

static Color_RGBA8 sBubblePrimColors[] = {
    { 255, 255, 255, 255 },
    { 150, 255, 255, 255 },
    { 100, 255, 255, 255 },
};

static Color_RGBA8 sBubbleEnvColors[] = {
    { 150, 150, 150, 0 },
    { 0, 100, 0, 255 },
    { 0, 0, 255, 255 },
};

void EnDragon_SpawnBubbles(EnDragon* this, PlayState* play, Vec3f basePos) {
    static Vec3f sBubbleVelocity = { 0.0f, 0.0f, 0.0f };
    static Vec3f sBubbleAccel = { 0.0f, 0.1f, 0.0f };
    s32 bubbleCount;
    s16 colorIndex;
    s16 scale;
    Vec3f bubblePos;
    s32 i;

    bubbleCount = (s32)Rand_CenteredFloat(5.0f) + 10;
    colorIndex = 0;
    if (this->action == DEEP_PYTHON_ACTION_DEAD) {
        colorIndex = 1;
        bubbleCount = (s32)Rand_CenteredFloat(5.0f) + 10;
    }

    for (i = 0; i < bubbleCount; i++) {
        Math_Vec3f_Copy(&bubblePos, &basePos);
        sBubbleVelocity.x = Rand_ZeroFloat(1.0f) * 23.0f;
        sBubbleVelocity.y = Rand_ZeroFloat(1.0f) * 10.0f;
        sBubbleVelocity.z = Rand_ZeroFloat(1.0f) * 23.0f;
        bubblePos.x += Rand_CenteredFloat(i * 30.0f);
        bubblePos.y += Rand_CenteredFloat(5.0f);
        bubblePos.z += Rand_CenteredFloat(i * 30.0f);
        sBubbleAccel.y = Rand_ZeroFloat(1.0f) * 20.0f * 3.0f;
        scale = Rand_S16Offset(380, 240);
        EffectSsDtBubble_SpawnCustomColor(play, &bubblePos, &sBubbleVelocity, &sBubbleAccel,
                                          &sBubblePrimColors[colorIndex], &sBubbleEnvColors[colorIndex], scale, 30,
                                          false);
    }
}

void EnDragon_RetreatOnceTimerEnds(EnDragon* this, PlayState* play) {
    if (this->timer == 0) {
        func_800B8D50(play, &this->actor, 10.0f, this->actor.world.rot.y, 10.0f, 8);
        EnDragon_SetupRetreatOrIdle(this);
    }
}

void EnDragon_SetupRetreatOrIdle(EnDragon* this) {
    EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_IDLE);
    this->state = 0;
    this->unk_2CC = 0;
    this->hasGrabbedPlayer = false;
    this->grabTimer = 0;
    this->timer = 30;
    this->actionFunc = EnDragon_RetreatOrIdle;
}

void EnDragon_RetreatOrIdle(EnDragon* this, PlayState* play) {
    SkelAnime_Update(&this->skelAnime);
    if (this->action == DEEP_PYTHON_ACTION_EXTEND) {
        EnDragon_SetupExtend(this);
    } else if ((this->timer != 0) && (fabsf(this->actor.world.pos.x - this->actor.home.pos.x) > 101.0f) &&
               (fabsf(this->actor.world.pos.z - this->actor.home.pos.z) > 101.0f)) {
        this->actor.speed = -100.0f;
    } else {
        this->actor.speed = 0.0f;

        if ((fabsf(this->actor.world.pos.x - this->actor.home.pos.x) > 4.0f) &&
            (fabsf(this->actor.world.pos.z - this->actor.home.pos.z) > 4.0f)) {
            Math_ApproachF(&this->actor.world.pos.x, this->actor.home.pos.x, 0.3f, 200.0f);
            Math_ApproachF(&this->actor.world.pos.z, this->actor.home.pos.z, 0.3f, 200.0f);
        } else if (this->action != DEEP_PYTHON_ACTION_IDLE) {
            this->action = DEEP_PYTHON_ACTION_IDLE;
        }
    }
}

void EnDragon_SetupExtend(EnDragon* this) {
    this->state = 0;
    this->behindJawRetreatTimer = this->state;
    Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_APPEAR_TRG);
    this->retreatTimer = 250;
    this->eoSnapOffset = 0.0f;
    this->eoSnapTimer = 0;
    this->eoSpitTimer = 60;

    // Deep Python Overhaul: the last python left awakens as a miniboss
    if (EO_PYTHON_ON() && !this->eoIsLast && (sNumPythonsDead >= (BREG(39) + 7))) {
        this->eoIsLast = true;
        this->actor.colChkInfo.health = 12;
        this->scale = 0.6f;
    }
    if (this->eoIsLast && !this->eoMusic) {
        this->eoMusic = true;
        Audio_PlayBgm_StorePrevBgm(NA_BGM_MINI_BOSS);
    }

    this->actionFunc = EnDragon_Extend;
}

/**
 * Deep Python Overhaul: a volley of water from the jaws (five shots for the last python).
 */
void EnDragon_EoSpit(EnDragon* this, PlayState* play) {
    EoShot_OrbFan(play, &this->actor, BO_ORB_WATER, &this->jawPos, 16.0f, this->eoIsLast ? 5 : 3, 0x1000);
    Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BITE);
}

/**
 * Deep Python Overhaul, burrow hunters: from its burrow, the python snaps forward to bite whoever is in front of it.
 */
void EnDragon_EoBurrowAttacks(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f extendedPos;
    f32 distToJaw = Math_Vec3f_DistXYZ(&this->jawPos, &player->actor.world.pos);
    s16 yawToPlayer = ABS_ALT(BINANG_SUB(Math_Vec3f_Yaw(&this->jawPos, &player->actor.world.pos),
                                         this->actor.shape.rot.y));

    if (!EO_PYTHON_ON() || !Eo_CanAttack(play)) {
        return;
    }

    if (this->eoSpitTimer > 0) {
        this->eoSpitTimer--;
    } else if ((yawToPlayer < 0x4000) && (distToJaw < 900.0f) && (this->eoSnapTimer == 0)) {
        EnDragon_EoSpit(this, play);
        this->eoSpitTimer = this->eoIsLast ? 80 : 110;
    }

    if (this->eoSnapCooldown > 0) {
        this->eoSnapCooldown--;
    }

    if ((this->eoSnapTimer == 0) && (this->eoSnapCooldown == 0) && (yawToPlayer < 0x3000) && (distToJaw < 380.0f)) {
        this->eoSnapTimer = 20;
        this->eoSnapCooldown = 70;
        this->jawZRotation = 0x1400;
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_APPEAR_TRG);
    }

    if (this->eoSnapTimer > 0) {
        this->eoSnapTimer--;
        // 8 frames out, then 12 back
        if (this->eoSnapTimer >= 12) {
            Math_ApproachF(&this->eoSnapOffset, 220.0f, 0.5f, 45.0f);
        } else {
            Math_ApproachF(&this->eoSnapOffset, 0.0f, 0.3f, 25.0f);
            this->jawZRotation = 0;
        }

        if ((this->eoSnapTimer >= 10) && (distToJaw < 110.0f)) {
            Eo_HurtPlayer(play, &this->actor, 0x10, this->actor.shape.rot.y, 8.0f, false, false);
            Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BITE);
        }

        Math_Vec3f_Copy(&extendedPos, &this->burrowEntrancePos);
        extendedPos.x += Math_SinS(this->actor.world.rot.y) * (-530.0f + this->eoSnapOffset);
        extendedPos.z += Math_CosS(this->actor.world.rot.y) * (-530.0f + this->eoSnapOffset);
        this->actor.world.pos.x = extendedPos.x;
        this->actor.world.pos.z = extendedPos.z;
    }
}

/**
 * Deep Python Overhaul, free swimming: loops in front of its den, spitting water and lunging at Link.
 */
void EnDragon_EoSetupSwim(EnDragon* this) {
    this->eoMode = EN_DRAGON_EO_SWIM;
    this->eoWasFree = true;
    this->eoSwimTimer = this->eoIsLast ? 900 : 600;
    this->eoSwimAngle = this->actor.home.rot.y;
    this->eoSpitTimer = 50;
    this->eoSnapTimer = 0;
    this->eoSnapCooldown = 90;
    this->eoDenCenter = this->burrowEntrancePos;
    this->eoDenCenter.x += Math_SinS(this->actor.home.rot.y) * 450.0f;
    this->eoDenCenter.z += Math_CosS(this->actor.home.rot.y) * 450.0f;
    if (this->animIndex != DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY) {
        EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY);
    }
    this->actionFunc = EnDragon_EoSwim;
}

void EnDragon_EoSwim(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f target;
    s16 turn;

    SkelAnime_Update(&this->skelAnime);
    this->eoWave += 0x700;
    if ((this->eoWave & 0x1FFF) < 0x700) {
        EnDragon_SpawnBubbles(this, play, this->jawPos);
    }

    if (this->eoMode == EN_DRAGON_EO_RETURN) {
        Vec3f extendedPos;

        Math_Vec3f_Copy(&extendedPos, &this->burrowEntrancePos);
        extendedPos.x += Math_SinS(this->actor.home.rot.y) * -530.0f;
        extendedPos.z += Math_CosS(this->actor.home.rot.y) * -530.0f;
        this->actor.speed = 0.0f;
        Math_ScaledStepToS(&this->actor.world.rot.y, this->actor.home.rot.y, 0x300);
        Math_ApproachF(&this->actor.world.pos.x, extendedPos.x, 0.2f, 30.0f);
        Math_ApproachF(&this->actor.world.pos.y, extendedPos.y, 0.2f, 30.0f);
        Math_ApproachF(&this->actor.world.pos.z, extendedPos.z, 0.2f, 30.0f);

        if ((Math_Vec3f_DistXYZ(&this->actor.world.pos, &extendedPos) < 30.0f) &&
            (ABS_ALT(BINANG_SUB(this->actor.world.rot.y, this->actor.home.rot.y)) < 0x300)) {
            this->actor.world.pos = extendedPos;
            this->actor.world.rot.y = this->actor.home.rot.y;
            this->eoMode = EN_DRAGON_EO_NONE;
            this->action = DEEP_PYTHON_ACTION_RETREAT;
            Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BACK);
            EnDragon_SetupRetreatOrIdle(this);
        }
        return;
    }

    // Home when hurt, when Link swims off, or after a while
    if (this->eoSwimTimer > 0) {
        this->eoSwimTimer--;
    }
    if ((this->action >= DEEP_PYTHON_ACTION_DAMAGE) || (this->eoSwimTimer == 0) || !EO_PYTHON_FREE_ON() ||
        (Math_Vec3f_DistXYZ(&player->actor.world.pos, &this->eoDenCenter) > 1400.0f)) {
        this->eoMode = EN_DRAGON_EO_RETURN;
        return;
    }

    if (this->eoSnapTimer > 0) {
        // Lunge at Link
        this->eoSnapTimer--;
        target = player->actor.focus.pos;
        turn = 0x480;
        Math_ApproachF(&this->actor.speed, 20.0f, 1.0f, 3.0f);
        this->jawZRotation = (this->eoSnapTimer > 8) ? 0x1400 : 0;
        if ((this->eoSnapTimer > 6) && (Math_Vec3f_DistXYZ(&this->jawPos, &player->actor.world.pos) < 110.0f)) {
            Eo_HurtPlayer(play, &this->actor, 0x10, this->actor.world.rot.y, 8.0f, false, false);
            Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BITE);
            this->eoSnapTimer = MIN(this->eoSnapTimer, 6);
        }
    } else {
        // Loop in front of the den
        this->eoSwimAngle += this->eoIsLast ? 0x160 : 0x110;
        target.x = this->eoDenCenter.x + (Math_SinS(this->eoSwimAngle) * 380.0f);
        target.y = this->eoDenCenter.y + (Math_SinS(this->eoSwimAngle * 2) * 110.0f);
        target.z = this->eoDenCenter.z + (Math_CosS(this->eoSwimAngle) * 380.0f);
        turn = 0x220;
        Math_ApproachF(&this->actor.speed, this->eoIsLast ? 10.0f : 8.0f, 1.0f, 0.5f);
        this->jawZRotation = 0;

        if (this->eoSpitTimer > 0) {
            this->eoSpitTimer--;
        } else if (Eo_CanAttack(play) && (Math_Vec3f_DistXYZ(&this->jawPos, &player->actor.world.pos) < 1100.0f)) {
            if (Rand_ZeroOne() < 0.5f) {
                EnDragon_EoSpit(this, play);
            } else {
                this->eoSnapTimer = 30;
                Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_APPEAR_TRG);
            }
            this->eoSpitTimer = this->eoIsLast ? 90 : 130;
        }
    }

    // Never stray far from the den
    if (Math_Vec3f_DistXYZ(&this->jawPos, &this->eoDenCenter) > 800.0f) {
        target = this->eoDenCenter;
        turn = 0x400;
    }

    Math_ScaledStepToS(&this->actor.world.rot.y, Math_Vec3f_Yaw(&this->jawPos, &target), turn);
    Math_ApproachF(&this->actor.world.pos.y, this->actor.world.pos.y + (target.y - this->jawPos.y), 0.08f, 6.0f);
}

void EnDragon_Extend(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 curFrame = this->skelAnime.curFrame;
    s16 yaw;

    EnDragon_SpawnBubbles(this, play, this->jawPos);

    if (this->action >= DEEP_PYTHON_ACTION_DAMAGE) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BACK);
        EnDragon_SetupRetreatOrIdle(this);
    } else if (this->retreatTimer == 0) {
        this->action = DEEP_PYTHON_ACTION_RETREAT;
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BACK);
        EnDragon_SetupRetreatOrIdle(this);
    } else if (this->state == DEEP_PYTHON_EXTEND_STATE_NOT_FULLY_EXTENDED) {
        Vec3f extendedPos;

        Math_Vec3f_Copy(&extendedPos, &this->burrowEntrancePos);
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_APPEAR - SFX_FLAG);
        extendedPos.x += Math_SinS(this->actor.world.rot.y) * -530.0f;
        extendedPos.z += Math_CosS(this->actor.world.rot.y) * -530.0f;
        this->actor.speed = 40.0f;
        Math_SmoothStepToS(&this->jawZRotation, 0xFA0, 5, 0xBB8, 0x14);

        if ((fabsf(this->actor.world.pos.x - extendedPos.x) < 51.0f) &&
            (fabsf(this->actor.world.pos.z - extendedPos.z) < 51.0f)) {
            this->actor.speed = 0.0f;
            Math_ApproachF(&this->actor.world.pos.x, extendedPos.x, 0.3f, 50.0f);
            Math_ApproachF(&this->actor.world.pos.z, extendedPos.z, 0.3f, 50.0f);
            if ((fabsf(this->actor.world.pos.x - extendedPos.x) < 4.0f) &&
                (fabsf(this->actor.world.pos.z - extendedPos.z) < 4.0f)) {
                if (this->animIndex != DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY) {
                    EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY);
                }

                this->state = DEEP_PYTHON_EXTEND_STATE_FULLY_EXTENDED;
            }
        }
    } else {
        Math_SmoothStepToS(&this->jawZRotation, 0, 5, 0xBB8, 0x14);
        SkelAnime_Update(&this->skelAnime);
        if (this->state == DEEP_PYTHON_EXTEND_STATE_FULLY_EXTENDED) {
            if (EO_PYTHON_FREE_ON()) {
                // Deep Python Overhaul: out of the burrow and into open water
                EnDragon_EoSetupSwim(this);
                return;
            }
            if (curFrame < this->animEndFrame) {
                return;
            }

            this->state = DEEP_PYTHON_EXTEND_STATE_REPEAT_LARGE_SWAY;
        }

        EnDragon_EoBurrowAttacks(this, play);

        yaw = ABS_ALT(BINANG_SUB(Math_Vec3f_Yaw(&this->jawPos, &player->actor.world.pos), this->actor.shape.rot.y));
        if (yaw < 0x5000) {
            // Player is in front of the jaw
            if ((curFrame >= this->animEndFrame) && (this->largeSwayWaitTimer == 0)) {
                if (this->animIndex != DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY) {
                    EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY);
                }

                this->state = DEEP_PYTHON_EXTEND_STATE_REPEAT_LARGE_SWAY;
            }

            this->behindJawRetreatTimer = 0;
        } else {
            // Player is in behind the jaw
            if (this->state == DEEP_PYTHON_EXTEND_STATE_REPEAT_LARGE_SWAY) {
                EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_SMALL_SIDE_SWAY);
                this->largeSwayWaitTimer = Rand_ZeroFloat(20.0f) + this->animEndFrame;
                this->state = DEEP_PYTHON_EXTEND_STATE_REPEAT_SMALL_SWAY;
            }

            this->behindJawRetreatTimer++;
            if (this->behindJawRetreatTimer > 60) {
                this->action = DEEP_PYTHON_ACTION_RETREAT;
                Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BACK);
                EnDragon_SetupRetreatOrIdle(this);
            }
        }
    }
}

void EnDragon_SetSubCamEyeAt(EnDragon* this, PlayState* play, Vec3f subCamEye, Vec3f subCamAt) {
    this->subCamId = CutsceneManager_GetCurrentSubCamId(this->actor.csId);
    Math_Vec3f_Copy(&this->subCamEye, &subCamEye);
    Math_Vec3f_Copy(&this->subCamAt, &subCamAt);
    Play_SetCameraAtEye(play, this->subCamId, &this->subCamAt, &this->subCamEye);
}

void EnDragon_SetupGrab(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f extendedPos;
    s16 yaw;

    if (!CutsceneManager_IsNext(this->grabCsId)) {
        CutsceneManager_Queue(this->grabCsId);
    } else {
        CutsceneManager_StartWithPlayerCs(this->grabCsId, &this->actor);
        Math_Vec3f_Copy(&extendedPos, &this->burrowEntrancePos);
        extendedPos.x += Math_SinS(this->actor.world.rot.y) * -530.0f;
        extendedPos.z += Math_CosS(this->actor.world.rot.y) * -530.0f;
        Math_Vec3f_Copy(&this->actor.world.pos, &extendedPos);
        yaw = Math_Vec3f_Yaw(&player->actor.world.pos, &this->jawPos);
        player->actor.shape.rot.y = yaw;
        player->actor.world.rot.y = yaw;
        this->state = DEEP_PYTHON_GRAB_STATE_START;
        this->grabTimer = 0;
        this->hasGrabbedPlayer = false;
        EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_IDLE);
        this->actionFunc = EnDragon_Grab;
    }
}

static Vec3f sSubCamEyePerPython[] = {
    { 1600.0f, 0.0f, 1400.0f }, { 1400.0f, 0.0f, 400.0f },  { 1800.0f, 0.0f, 1400.f },  { 1100.0f, -200.0f, 1500.0f },
    { 2000.0f, 0.0f, 1500.0f }, { 1900.0f, 0.0f, 1800.0f }, { 1700.0f, 0.0f, 1100.0f }, { 1700.0f, 0.0f, 1100.0f },
};

static Vec3f sSubCamAtPerPython[] = {
    { 300.0f, -100.0f, 1300.0f }, { 1500.0f, 0.0f, 2400.0f }, { 300.0f, -100.0f, 1300.0f }, { 1900.0f, 500.0f, 600.0f },
    { -1000.0f, 0.0f, 1000.0f },  { 1200.0f, 0.0f, 1500.0f }, { 1100.0f, 0.0f, 2000.0f },   { 1100.0f, 0.0f, 2000.0f },
};

static s16 sZRotPerPython[] = { 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x0BB8, 0x0000, 0x0000 };

static s32 sMaxGrabTimerPerPython[] = { 5, 5, 5, 4, 5, 8, 5, 5 };

void EnDragon_Grab(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Vec3f pos; // used as both the extended position and the camera eye
    Vec3f subCamAt;

    this->subCamId = CutsceneManager_GetCurrentSubCamId(this->actor.csId);
    SkelAnime_Update(&this->skelAnime);

    if (this->grabTimer == 0) {
        if (!this->hasGrabbedPlayer) {
            Math_Vec3f_Copy(&player->actor.world.pos, &this->playerGrabPosition);
            Math_Vec3f_Copy(&this->playerGrabPositionTemp, &this->playerGrabPosition);
            this->hasGrabbedPlayer = true;
        } else {
            Math_Vec3f_Copy(&player->actor.world.pos, &this->playerGrabPositionTemp);
        }

        Math_Vec3f_Copy(&pos, &this->burrowEntrancePos);
        pos.x += Math_SinS(this->actor.world.rot.y) * -930.0f;
        pos.z += Math_CosS(this->actor.world.rot.y) * -930.0f;
        Math_Vec3f_Copy(&this->actor.world.pos, &pos);
        this->jawZRotation = 0x1450;
        this->actor.speed = 60.0f;
    }

    this->grabTimer++;
    Math_SmoothStepToS(&this->actor.shape.rot.z, sZRotPerPython[this->pythonIndex], 0xA, 0x1F4, 0x14);
    EnDragon_SpawnBubbles(this, play, this->jawPos);

    Math_Vec3f_Copy(&pos, &this->burrowEntrancePos);

    pos.x += Math_SinS(this->actor.world.rot.y) * sSubCamEyePerPython[this->pythonIndex].x;
    pos.y += sSubCamEyePerPython[this->pythonIndex].y;
    pos.z += Math_CosS(this->actor.world.rot.y) * sSubCamEyePerPython[this->pythonIndex].z;

    Math_Vec3f_Copy(&subCamAt, &this->actor.world.pos);
    subCamAt.x += Math_SinS(this->actor.world.rot.y) * sSubCamAtPerPython[this->pythonIndex].x;
    subCamAt.y += sSubCamAtPerPython[this->pythonIndex].y;
    subCamAt.z += Math_CosS(this->actor.world.rot.y) * sSubCamAtPerPython[this->pythonIndex].z;

    EnDragon_SetSubCamEyeAt(this, play, pos, subCamAt);

    if (this->grabTimer > sMaxGrabTimerPerPython[this->pythonIndex]) {
        if (this->state == DEEP_PYTHON_GRAB_STATE_START) {
            Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
            this->state = DEEP_PYTHON_GRAB_STATE_GRABBED;
        }

        play->unk_18770(play, player);
        player->actor.parent = &this->actor;
        player->av2.actionVar2 = 50;
        this->action = DEEP_PYTHON_ACTION_GRAB;
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_EAT);
        EnDragon_SetupAttack(this);
    }
}

void EnDragon_SetupAttack(EnDragon* this) {
    if (this->animIndex != DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY) {
        EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY);
    }

    this->behindJawRetreatTimer = 0;
    this->grabTimer = 0;
    this->state = DEEP_PYTHON_ATTACK_STATE_START;
    this->actionFunc = EnDragon_Attack;
}

void EnDragon_Attack(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 curFrame = this->skelAnime.curFrame;
    Vec3f pos; // used as both the extended position and the camera eye
    Vec3f subCamAt;

    SkelAnime_Update(&this->skelAnime);
    Math_SmoothStepToS(&this->actor.shape.rot.z, 0, 0xA, 0x1388, 0);
    if ((play->gameplayFrames % 16) == 0) {
        play->damagePlayer(play, -2);

        //! @bug: This function should only pass Player*: it uses *(this + 0x153), which is meant to be
        //! player->currentMask, but in this case is garbage in the skelAnime
        Player_PlaySfx((Player*)&this->actor, player->ageProperties->voiceSfxIdOffset + NA_SE_VO_LI_DAMAGE_S);
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BITE);
        CollisionCheck_GreenBlood(play, NULL, &player->actor.world.pos);
    }

    Math_Vec3f_Copy(&pos, &this->actor.world.pos);
    pos.x += Math_SinS(this->actor.world.rot.y) * 3000.0f;
    pos.y += 600.0f;
    pos.z += Math_CosS(this->actor.world.rot.y) * 3000.0f;

    Math_Vec3f_Copy(&subCamAt, &this->actor.world.pos);
    subCamAt.x += Math_SinS(this->actor.world.rot.y) * 1200.0f;
    subCamAt.y += -100.0f;
    subCamAt.z += Math_CosS(this->actor.world.rot.y) * 1200.0f;

    EnDragon_SetSubCamEyeAt(this, play, pos, subCamAt);

    player->actor.world.rot.y = player->actor.shape.rot.y = this->actor.world.rot.y;
    player->actor.world.rot.x = player->actor.shape.rot.x = this->actor.world.rot.x;
    player->actor.world.rot.z = player->actor.shape.rot.z = this->actor.world.rot.z - 0x36B0;
    Math_Vec3f_Copy(&player->actor.world.pos, &this->playerGrabPosition);
    this->jawZRotation = 0xC8;

    Math_Vec3f_Copy(&pos, &this->burrowEntrancePos);
    pos.x += Math_SinS(this->actor.world.rot.y) * -530.0f;
    pos.z += Math_CosS(this->actor.world.rot.y) * -530.0f;
    Math_ApproachF(&this->actor.world.pos.x, pos.x, 0.3f, 200.0f);
    Math_ApproachF(&this->actor.world.pos.y, pos.y, 0.3f, 200.0f);
    Math_ApproachF(&this->actor.world.pos.z, pos.z, 0.3f, 200.0f);

    if ((this->state <= DEEP_PYTHON_ATTACK_STATE_START) && (curFrame >= this->animEndFrame)) {
        Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_BITE);
        if (this->animIndex != DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY) {
            EnDragon_ChangeAnim(this, DEEP_PYTHON_ANIM_LARGE_SIDE_SWAY);
        }

        this->state++;
    }

    if (((this->state != DEEP_PYTHON_ATTACK_STATE_START) && (curFrame >= this->animEndFrame)) ||
        !(player->stateFlags2 & PLAYER_STATE2_80) || (this->collider.elements[0].base.acElemFlags & ACELEM_HIT) ||
        (this->collider.elements[1].base.acElemFlags & ACELEM_HIT) ||
        (this->collider.elements[2].base.acElemFlags & ACELEM_HIT)) {
        player->actor.parent = NULL;
        this->grabWaitTimer = 30;
        CutsceneManager_Stop(this->grabCsId);
        if (player->stateFlags2 & PLAYER_STATE2_80) {
            player->av2.actionVar2 = 100;
        }

        this->actor.flags &= ~ACTOR_FLAG_FREEZE_EXCEPTION;

        if ((this->state != DEEP_PYTHON_ATTACK_STATE_START) && (curFrame >= this->animEndFrame)) {
            this->timer = 3;
            this->actionFunc = EnDragon_RetreatOnceTimerEnds;
        } else {
            EnDragon_SetupRetreatOrIdle(this);
        }
    }
}

void EnDragon_SetupDead(EnDragon* this, PlayState* play) {
    if (!CutsceneManager_IsNext(this->deathCsId)) {
        CutsceneManager_Queue(this->deathCsId);
    } else {
        CutsceneManager_StartWithPlayerCs(this->deathCsId, &this->actor);
        this->animEndFrame = Animation_GetLastFrame(&gDeepPythonSmallSideSwayAnim);
        Animation_Change(&this->skelAnime, &gDeepPythonSmallSideSwayAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, 0.0f);
        this->timer = 20;
        this->actionFunc = EnDragon_Dead;
    }
}

void EnDragon_Dead(EnDragon* this, PlayState* play) {
    Vec3f seahorsePos;

    SkelAnime_Update(&this->skelAnime);
    this->actor.shape.rot.z += 0x1000;
    this->jawZRotation = 0xFA0;
    EnDragon_SpawnBubbles(this, play, this->jawPos);

    // Deep Python Overhaul: a python that died in open water sinks back to its den first
    if (this->eoWasFree && (Math_Vec3f_DistXYZ(&this->actor.world.pos, &this->actor.home.pos) > 30.0f)) {
        this->actor.speed = 0.0f;
        Math_ApproachF(&this->actor.world.pos.x, this->actor.home.pos.x, 0.25f, 120.0f);
        Math_ApproachF(&this->actor.world.pos.y, this->actor.home.pos.y, 0.25f, 120.0f);
        Math_ApproachF(&this->actor.world.pos.z, this->actor.home.pos.z, 0.25f, 120.0f);
        this->timer = 0;
        return;
    }

    if ((this->timer != 0) && (fabsf(this->actor.world.pos.x - this->actor.home.pos.x) > 121.0f) &&
        (fabsf(this->actor.world.pos.z - this->actor.home.pos.z) > 121.0f)) {
        this->actor.speed = -120.0f;
        if (((this->pythonIndex & 1) == 0) && (Rand_ZeroOne() < 0.5f)) {
            //! @bug: !play->gameplayFrames is 0 essentially all the time, so this code never runs.
            if (((!play->gameplayFrames) & 0x1F)) {
                Item_DropCollectibleRandom(play, NULL, &this->jawPos, 0x90);
            }
        }

        this->action = DEEP_PYTHON_ACTION_DEAD;
        return;
    }

    this->actor.speed = 0.0f;
    if ((fabsf(this->actor.world.pos.x - this->actor.home.pos.x) > 20.0f) &&
        (fabsf(this->actor.world.pos.z - this->actor.home.pos.z) > 20.0f)) {
        Math_ApproachF(&this->actor.world.pos.x, this->actor.home.pos.x, 0.3f, 300.0f);
        Math_ApproachF(&this->actor.world.pos.z, this->actor.home.pos.z, 0.3f, 300.0f);
        return;
    }

    sNumPythonsDead++;
    if (sNumPythonsDead >= (BREG(39) + 8)) {
        Math_Vec3f_Copy(&seahorsePos, &this->actor.parent->world.pos);
        seahorsePos.x += Math_SinS(this->actor.parent->world.rot.y + 0x8000) * (500.0f + BREG(38));
        seahorsePos.y += -100.0f + BREG(33);
        seahorsePos.z += Math_CosS(this->actor.parent->world.rot.y + 0x8000) * (500.0f + BREG(38));
        if (Actor_SpawnAsChildAndCutscene(&play->actorCtx, play, ACTOR_EN_OT, seahorsePos.x, seahorsePos.y,
                                          seahorsePos.z, 0, this->actor.shape.rot.y, 0,
                                          SEAHORSE_PARAMS(SEAHORSE_TYPE_1, 0, 0), this->actor.csId,
                                          this->actor.halfDaysBits, NULL)) {
            SET_WEEKEVENTREG(WEEKEVENTREG_13_01);
            switch (this->pythonIndex) {
                case 0:
                    SET_WEEKEVENTREG(WEEKEVENTREG_83_10);
                    break;

                case 1:
                    SET_WEEKEVENTREG(WEEKEVENTREG_83_20);
                    break;

                case 2:
                    SET_WEEKEVENTREG(WEEKEVENTREG_83_40);
                    break;

                case 3:
                    SET_WEEKEVENTREG(WEEKEVENTREG_83_80);
                    break;
                case 4:
                    SET_WEEKEVENTREG(WEEKEVENTREG_84_01);
                    break;

                case 5:
                    SET_WEEKEVENTREG(WEEKEVENTREG_84_02);
                    break;

                case 6:
                    SET_WEEKEVENTREG(WEEKEVENTREG_84_04);
                    break;

                case 7:
                    SET_WEEKEVENTREG(WEEKEVENTREG_84_08);
                    break;
            }
        }
    }

    Actor_Kill(&this->actor);
}

void EnDragon_UpdateDamage(EnDragon* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    PlayerImpactType playerImpactType;

    if (this->action == DEEP_PYTHON_ACTION_EXTEND) {
        if ((this->collider.elements[2].base.acElemFlags & ACELEM_HIT) ||
            (this->collider.elements[3].base.acElemFlags & ACELEM_HIT) ||
            (this->collider.elements[4].base.acElemFlags & ACELEM_HIT) ||
            (this->collider.elements[5].base.acElemFlags & ACELEM_HIT) ||
            (this->collider.elements[6].base.acElemFlags & ACELEM_HIT) ||
            (this->collider.elements[7].base.acElemFlags & ACELEM_HIT)) {
            Actor_ApplyDamage(&this->actor);
            Actor_SetColorFilter(&this->actor, COLORFILTER_COLORFLAG_RED, 255, COLORFILTER_BUFFLAG_OPA, 25);
            if (this->actor.colChkInfo.health > 0) {
                Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_DAMAGE);
                this->action = DEEP_PYTHON_ACTION_DAMAGE;
            } else {
                Enemy_StartFinishingBlow(play, &this->actor);
                if (this->eoMode != EN_DRAGON_EO_NONE) {
                    // Deep Python Overhaul: back on its den's axis for the vanilla death
                    this->actor.world.rot.y = this->actor.home.rot.y;
                    this->actor.speed = 0.0f;
                    this->eoMode = EN_DRAGON_EO_NONE;
                }
                if (this->eoMusic) {
                    this->eoMusic = false;
                    Audio_RestorePrevBgm();
                }
                Actor_PlaySfx(&this->actor, NA_SE_EN_UTSUBO_DEAD);
                this->actor.flags |= ACTOR_FLAG_LOCK_ON_DISABLED;
                this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
                this->actor.flags |= ACTOR_FLAG_FREEZE_EXCEPTION;
                this->action = DEEP_PYTHON_ACTION_SETUP_DEAD;
                this->actionFunc = EnDragon_SetupDead;
            }
        }
    }

    if ((this->action == DEEP_PYTHON_ACTION_EXTEND) && (this->grabWaitTimer == 0) &&
        (player->invincibilityTimer == 0) && (this->collider.elements[0].base.ocElemFlags & OCELEM_HIT) &&
        !((Actor_GetPlayerImpact(play, 1000.0f, &this->actor.world.pos, &playerImpactType) >= 0.0f) &&
          (playerImpactType == PLAYER_IMPACT_ZORA_BARRIER))) {
        if (this->eoMode != EN_DRAGON_EO_NONE) {
            // Deep Python Overhaul: a grab in open water drags Link back to the den for the vanilla bite
            this->actor.world.rot.y = this->actor.home.rot.y;
            this->eoMode = EN_DRAGON_EO_NONE;
        }
        this->eoSnapTimer = 0;
        this->eoSnapOffset = 0.0f;
        this->actor.speed = 0.0f;
        this->action = DEEP_PYTHON_ACTION_GRAB;
        this->actor.flags |= ACTOR_FLAG_FREEZE_EXCEPTION;
        this->actionFunc = EnDragon_SetupGrab;
    }
}

void EnDragon_Update(Actor* thisx, PlayState* play) {
    s32 pad;
    EnDragon* this = (EnDragon*)thisx;

    if (this->retreatTimer != 0) {
        this->retreatTimer--;
    }

    if (this->timer != 0) {
        this->timer--;
    }

    if (this->largeSwayWaitTimer != 0) {
        this->largeSwayWaitTimer--;
    }

    if (this->grabWaitTimer != 0) {
        this->grabWaitTimer--;
    }

    EnDragon_UpdateDamage(this, play);
    this->actor.shape.rot.y = this->actor.world.rot.y;
    Math_Vec3f_Copy(&this->actor.focus.pos, &this->focusPos);
    Math_Vec3s_Copy(&this->actor.focus.rot, &this->actor.world.rot);
    Actor_SetScale(&this->actor, this->scale);

    this->actionFunc(this, play);
    Actor_MoveWithGravity(&this->actor);

    if (this->action != DEEP_PYTHON_ACTION_GRAB) {
        CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
    }

    if (this->action < DEEP_PYTHON_ACTION_DAMAGE) {
        CollisionCheck_SetAC(play, &play->colChkCtx, &this->collider.base);
        CollisionCheck_SetAT(play, &play->colChkCtx, &this->collider.base);
    }
}

s32 EnDragon_OverrideLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* thisx) {
    EnDragon* this = (EnDragon*)thisx;

    if (limbIndex == DEEP_PYTHON_LIMB_JAW) {
        rot->x += this->jawXRotation;
        rot->y += this->jawYRotation;
        rot->z += this->jawZRotation;
    }

    // Deep Python Overhaul: the body ripples while it swims in open water
    if ((this->eoMode == EN_DRAGON_EO_SWIM) && (limbIndex >= DEEP_PYTHON_LIMB_BODY_SEGMENT_4) &&
        (limbIndex <= DEEP_PYTHON_LIMB_BODY_SEGMENT_1)) {
        rot->y += (s16)(Math_SinS(this->eoWave + (limbIndex * 0x2400)) * 0x500);
    }

    return false;
}

void EnDragon_PostLimbDraw(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* thisx) {
    EnDragon* this = (EnDragon*)thisx;
    Vec3f playerGrabOffsetFromJawPos = { 350.0f, -120.0f, -60.0f };

    if (limbIndex == DEEP_PYTHON_LIMB_JAW) {
        Matrix_MultVec3f(&gZeroVec3f, &this->jawPos);
        playerGrabOffsetFromJawPos.x = 350.0f;
        playerGrabOffsetFromJawPos.y = -120.0f;
        playerGrabOffsetFromJawPos.z = -60.0f;
        Matrix_MultVec3f(&playerGrabOffsetFromJawPos, &this->playerGrabPosition);
    }

    if (limbIndex == DEEP_PYTHON_LIMB_HEAD_AND_COLLAR_ROOT) {
        Matrix_MultVec3f(&gZeroVec3f, &this->focusPos);
    }

    Collider_UpdateSpheres(limbIndex, &this->collider);
}

/**
 * Deep Python Overhaul: the model was made to stay half inside a burrow, so for free swimming the last body segments
 * are tapered into a tail.
 */
void EnDragon_TransformLimbDraw(PlayState* play, s32 limbIndex, Actor* thisx) {
    if (!EO_PYTHON_FREE_ON()) {
        return;
    }

    switch (limbIndex) {
        case DEEP_PYTHON_LIMB_BODY_SEGMENT_5:
            Matrix_Scale(0.9f, 0.85f, 0.85f, MTXMODE_APPLY);
            break;
        case DEEP_PYTHON_LIMB_BODY_SEGMENT_6:
            Matrix_Scale(0.9f, 0.7f, 0.7f, MTXMODE_APPLY);
            break;
        case DEEP_PYTHON_LIMB_BODY_SEGMENT_7:
            Matrix_Scale(0.85f, 0.45f, 0.45f, MTXMODE_APPLY);
            break;
        default:
            break;
    }
}

void EnDragon_Draw(Actor* thisx, PlayState* play) {
    EnDragon* this = (EnDragon*)thisx;

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    SkelAnime_DrawTransformFlexOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable,
                                   this->skelAnime.dListCount, EnDragon_OverrideLimbDraw, EnDragon_PostLimbDraw,
                                   EnDragon_TransformLimbDraw, &this->actor);
}

void EnDragon_Reset(void) {
    sNumPythonsDead = 0;
}
