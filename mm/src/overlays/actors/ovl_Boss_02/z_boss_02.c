/*
 * File: z_boss_02.c
 * Overlay: ovl_Boss_02
 * Description: Twinmold
 */

#include "z_boss_02.h"
#include "z64rumble.h"
#include "z64shrink_window.h"
#include "overlays/actors/ovl_Door_Warp1/z_door_warp1.h"
#include "overlays/actors/ovl_En_Tanron5/z_en_tanron5.h"
#include "overlays/actors/ovl_Item_B_Heart/z_item_b_heart.h"
#include "objects/gameplay_keep/gameplay_keep.h"

#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Enhancements/BossOverhaul/BossOverhaul.h"

#define FLAGS                                                                                 \
    (ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE | ACTOR_FLAG_UPDATE_CULLING_DISABLED | \
     ACTOR_FLAG_DRAW_CULLING_DISABLED)

void Boss02_Init(Actor* thisx, PlayState* play);
void Boss02_Destroy(Actor* thisx, PlayState* play);
void Boss02_Twinmold_Update(Actor* thisx, PlayState* play);
void Boss02_Twinmold_Draw(Actor* thisx, PlayState* play2);
void Boss02_Reset(void);

void func_809DAA74(Boss02* this, PlayState* play);
void func_809DAA98(Boss02* this, PlayState* play);
void func_809DAAA8(Boss02* this, PlayState* play);
void func_809DAB78(Boss02* this, PlayState* play);
void Boss02_Tail_Update(Actor* thisx, PlayState* play);
void Boss02_BattleHandler_Update(Actor* thisx, PlayState* play);
void Boss02_BattleHandler_Draw(Actor* thisx, PlayState* play);
void Boss02_UpdateEffects(PlayState* play);
void Boss02_DrawEffects(PlayState* play);
void Boss02_HandleGiantsMaskCutscene(Boss02* this, PlayState* play);
void func_809DEAC4(Boss02* this, PlayState* play);
// Boss Overhaul: the twin dragons
void Boss02_BoShotInit(Boss02* this, PlayState* play);
void Boss02_BoShotUpdate(Actor* thisx, PlayState* play);
void Boss02_BoShotDraw(Actor* thisx, PlayState* play);
void Boss02_BoDragonUpdate(Boss02* this, PlayState* play);
void Boss02_BoDrawMouth(Boss02* this, PlayState* play);
void Boss02_BoEruption(Boss02* this, PlayState* play, Vec3f* breachPos);
f32 Boss02_BoChaseChance(Boss02* this);
f32 Boss02_BoSpeedScale(Boss02* this);

typedef enum {
    /*  0 */ GIANTS_MASK_CS_STATE_WAITING_FOR_MASK, // Starts the cutscene when the player puts on/takes off the mask.
    /*  1 */ GIANTS_MASK_CS_STATE_MASK_ON,          // Controls the cutscene where the player puts the mask on.
    /*  2 */ GIANTS_MASK_CS_STATE_MASK_ON_SKIPPED,  // Moves to the "done" state when the mask-on cutscene is skipped.
    /* 10 */ GIANTS_MASK_CS_STATE_MASK_OFF = 10,    // Controls the cutscene where the player takes the mask off.
    /* 11 */ GIANTS_MASK_CS_STATE_MASK_OFF_SKIPPED, // Moves to the "done" state when the mask-off cutscene is skipped.
    /* 20 */ GIANTS_MASK_CS_STATE_DONE = 20         // Stops the cutscene and resets the player's scale back to normal.
} GiantsMaskCsState;

typedef enum {
    /* 0 */ GIANTS_MASK_CS_FLASH_STATE_NOT_STARTED,    // Does nothing at all.
    /* 1 */ GIANTS_MASK_CS_FLASH_STATE_STARTED,        // Plays the "mask flash" SFX and initializes a white FillScreen.
    /* 2 */ GIANTS_MASK_CS_FLASH_STATE_INCREASE_ALPHA, // Increases the alpha for the FillScreen on every frame.
    /* 3 */ GIANTS_MASK_CS_FLASH_STATE_DECREASE_ALPHA  // Decreases the alpha for the FillScreen on every frame.
} GiantsMaskCsFlashState;

/**
 * If set to true, the player can skip the cutscene of putting on the Giant's Mask
 * by pressing A, B, or any C-button. If false, they must watch it in full.
 */
u8 sCanSkipMaskOnCs;

/**
 * If set to true, the player can skip the cutscene of taking off the Giant's Mask
 * by pressing A, B, or any C-button. If false, they must watch it in full.
 */
u8 sCanSkipMaskOffCs;

/**
 * True when the player is giant-sized (i.e. wearing the Giant's Mask), false when the player is normal-sized.
 */
u8 sIsInGiantMode;

Boss02* sRedTwinmold;
Boss02* sBlueTwinmold;
Boss02* sTwinmoldBattleHandler;
u8 sTwinmoldMusicStartTimer;
DoorWarp1* sBlueWarp;
TwinmoldEffect sTwinmoldEffects[TWINMOLD_EFFECT_COUNT];

static DamageTable sBlueTwinmoldDamageTable = {
    /* Deku Nut       */ DMG_ENTRY(0, 0x0),
    /* Deku Stick     */ DMG_ENTRY(1, 0xF),
    /* Horse trample  */ DMG_ENTRY(0, 0x0),
    /* Explosives     */ DMG_ENTRY(1, 0xF),
    /* Zora boomerang */ DMG_ENTRY(0, 0x0),
    /* Normal arrow   */ DMG_ENTRY(1, 0xF),
    /* UNK_DMG_0x06   */ DMG_ENTRY(0, 0x0),
    /* Hookshot       */ DMG_ENTRY(0, 0x0),
    /* Goron punch    */ DMG_ENTRY(2, 0xF),
    /* Sword          */ DMG_ENTRY(1, 0xF),
    /* Goron pound    */ DMG_ENTRY(0, 0xF),
    /* Fire arrow     */ DMG_ENTRY(5, 0x2),
    /* Ice arrow      */ DMG_ENTRY(1, 0x3),
    /* Light arrow    */ DMG_ENTRY(2, 0x4),
    /* Goron spikes   */ DMG_ENTRY(1, 0xF),
    /* Deku spin      */ DMG_ENTRY(0, 0x0),
    /* Deku bubble    */ DMG_ENTRY(0, 0x0),
    /* Deku launch    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x12   */ DMG_ENTRY(0, 0x0),
    /* Zora barrier   */ DMG_ENTRY(0, 0x0),
    /* Normal shield  */ DMG_ENTRY(0, 0x0),
    /* Light ray      */ DMG_ENTRY(0, 0x0),
    /* Thrown object  */ DMG_ENTRY(1, 0xF),
    /* Zora punch     */ DMG_ENTRY(1, 0xF),
    /* Spin attack    */ DMG_ENTRY(1, 0xF),
    /* Sword beam     */ DMG_ENTRY(2, 0xF),
    /* Normal Roll    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1B   */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1C   */ DMG_ENTRY(0, 0x0),
    /* Unblockable    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1E   */ DMG_ENTRY(0, 0x0),
    /* Powder Keg     */ DMG_ENTRY(1, 0xF),
};

static DamageTable sRedTwinmoldDamageTable = {
    /* Deku Nut       */ DMG_ENTRY(0, 0x0),
    /* Deku Stick     */ DMG_ENTRY(1, 0xF),
    /* Horse trample  */ DMG_ENTRY(0, 0x0),
    /* Explosives     */ DMG_ENTRY(1, 0xF),
    /* Zora boomerang */ DMG_ENTRY(0, 0x0),
    /* Normal arrow   */ DMG_ENTRY(1, 0xF),
    /* UNK_DMG_0x06   */ DMG_ENTRY(0, 0x0),
    /* Hookshot       */ DMG_ENTRY(0, 0x0),
    /* Goron punch    */ DMG_ENTRY(2, 0xF),
    /* Sword          */ DMG_ENTRY(1, 0xF),
    /* Goron pound    */ DMG_ENTRY(0, 0xF),
    /* Fire arrow     */ DMG_ENTRY(1, 0x2),
    /* Ice arrow      */ DMG_ENTRY(5, 0x3),
    /* Light arrow    */ DMG_ENTRY(2, 0x4),
    /* Goron spikes   */ DMG_ENTRY(1, 0xF),
    /* Deku spin      */ DMG_ENTRY(0, 0x0),
    /* Deku bubble    */ DMG_ENTRY(0, 0x0),
    /* Deku launch    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x12   */ DMG_ENTRY(0, 0x0),
    /* Zora barrier   */ DMG_ENTRY(0, 0x0),
    /* Normal shield  */ DMG_ENTRY(0, 0x0),
    /* Light ray      */ DMG_ENTRY(0, 0x0),
    /* Thrown object  */ DMG_ENTRY(1, 0xF),
    /* Zora punch     */ DMG_ENTRY(1, 0xF),
    /* Spin attack    */ DMG_ENTRY(1, 0xF),
    /* Sword beam     */ DMG_ENTRY(2, 0xF),
    /* Normal Roll    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1B   */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1C   */ DMG_ENTRY(0, 0x0),
    /* Unblockable    */ DMG_ENTRY(0, 0x0),
    /* UNK_DMG_0x1E   */ DMG_ENTRY(0, 0x0),
    /* Powder Keg     */ DMG_ENTRY(1, 0xF),
};

ActorProfile Boss_02_Profile = {
    /**/ ACTOR_BOSS_02,
    /**/ ACTORCAT_BOSS,
    /**/ FLAGS,
    /**/ OBJECT_BOSS02,
    /**/ sizeof(Boss02),
    /**/ Boss02_Init,
    /**/ Boss02_Destroy,
    /**/ Boss02_Twinmold_Update,
    /**/ Boss02_Twinmold_Draw,
    /**/ Boss02_Reset,
};

/**
 * Multiplies the scale differently depending on whether the player is wearing the Giant's Mask or not.
 * When the player is wearing the Giant's Mask, this value is smaller to make the player seem larger.
 */
static f32 sGiantModeScaleFactor = 1.0f;

s16 D_809DF5B4[] = {
    0, 195, 190, 185, 180, 175, 170, 165, 160, 155, 150, 145, 140, 135, 130, 125, 120, 115, 110, 105, 100, 95, 90, 0,
};

s16 D_809DF5E4[] = {
    0, 196, 192, 188, 184, 180, 176, 172, 168, 164, 160, 156, 152, 148, 144, 140, 136, 132, 128, 124, 120, 116, 112,
};

static ColliderJntSphElementInit sJntSphElementsInit1[] = {
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 1, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 2, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 3, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 4, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 5, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 6, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 7, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 8, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 9, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 10, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 11, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 12, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 13, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 14, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 15, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 16, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 17, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 18, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 19, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 20, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 21, { { 0, 0, 0 }, 85 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 0, { { 0, 0, 0 }, 130 }, 100 },
    },
};

static ColliderJntSphInit sJntSphInit1 = {
    {
        COL_MATERIAL_METAL,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_HARD | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_1,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sJntSphElementsInit1),
    sJntSphElementsInit1,
};

static ColliderJntSphElementInit sJntSphElementsInit2[] = {
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 0, { { 0, 0, 0 }, 130 }, 100 },
    },
    {
        {
            ELEM_MATERIAL_UNK3,
            { 0xF7CFFFFF, 0x04, 0x10 },
            { 0xF7CFFFFF, 0x00, 0x00 },
            ATELEM_ON | ATELEM_SFX_NORMAL,
            ACELEM_ON,
            OCELEM_ON,
        },
        { 22, { { 0, 0, 0 }, 110 }, 100 },
    },
};

static ColliderJntSphInit sJntSphInit2 = {
    {
        COL_MATERIAL_HIT0,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_PLAYER,
        OC2_TYPE_1,
        COLSHAPE_JNTSPH,
    },
    ARRAY_COUNT(sJntSphElementsInit2),
    sJntSphElementsInit2,
};

static ColliderCylinderInit sCylinderInit = {
    {
        COL_MATERIAL_METAL,
        AT_ON | AT_TYPE_ENEMY,
        AC_ON | AC_HARD | AC_TYPE_PLAYER,
        OC1_ON | OC1_TYPE_ALL,
        OC2_TYPE_1,
        COLSHAPE_CYLINDER,
    },
    {
        ELEM_MATERIAL_UNK2,
        { 0xF7CFFFFF, 0x04, 0x00 },
        { 0xF7CFFFFF, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_ON,
        OCELEM_ON,
    },
    { 150, 200, 0, { 0, 0, 0 } },
};

Vec3f D_809DF9C0[] = {
    { 0.0f, -200.0f, 1000.0f },    { 0.0f, 500.0f, 1000.0f },        { 0.0f, 1000.0f, 1000.0f },
    { 1000.0f, 500.0f, 1000.0f },  { 1000.0f, 1000.0f, -1000.0f },   { -1000.0f, 500.0f, -1000.0f },
    { -1000.0f, 500.0f, 1000.0f }, { -1000.0f, -1000.0f, -1000.0f }, { -1000.0f, -1000.0f, -1000.0f },
};

Vec3f D_809DFA2C[] = {
    { 0.0f, -200.0f, -800.0f },  { 0.0f, 800.0f, -800.0f },   { 800.0f, 300.0f, -800.0f },
    { -800.0f, 800.0f, 0.0f },   { -800.0f, -1000.0f, 0.0f }, { -800.0f, -1000.0f, 0.0f },
    { -800.0f, -1000.0f, 0.0f }, { -800.0f, -1000.0f, 0.0f }, { -800.0f, -1000.0f, 0.0f },
};

void Boss02_FillScreen(PlayState* play, u8 red, u8 green, u8 blue, u8 alpha) {
    R_PLAY_FILL_SCREEN_ON = true;
    R_PLAY_FILL_SCREEN_R = red;
    R_PLAY_FILL_SCREEN_G = green;
    R_PLAY_FILL_SCREEN_B = blue;
    R_PLAY_FILL_SCREEN_ALPHA = alpha;
}

void Boss02_SetFillScreenAlpha(PlayState* play, u8 alpha) {
    R_PLAY_FILL_SCREEN_ALPHA = alpha;
}

void Boss02_StopFillScreen(PlayState* play) {
    R_PLAY_FILL_SCREEN_ON = false;
}

void Boss02_SpawnEffectSand(TwinmoldEffect* effects, Vec3f* pos, f32 scale) {
    s16 i;

    for (i = 0; i < TWINMOLD_EFFECT_COUNT; i++, effects++) {
        if (effects->type == TWINMOLD_EFFECT_NONE) {
            effects->type = TWINMOLD_EFFECT_SAND;
            effects->pos = *pos;
            effects->velocity.x = Rand_CenteredFloat(30.0f);
            effects->velocity.y = Rand_ZeroFloat(7.0f) + 7.0f;
            effects->velocity.z = Rand_CenteredFloat(30.0f);
            effects->accel.y = -0.3f;
            effects->scale = scale;
            effects->alpha = 255;
            effects->timer = 0;
            effects->targetScale = 2.0f * scale;
            effects->accel.x = effects->accel.z = 0.0f;
            effects->epoch++;
            break;
        }
    }
}

void Boss02_SpawnEffectFragment(TwinmoldEffect* effects, Vec3f* pos) {
    s16 i;

    for (i = 0; i < TWINMOLD_EFFECT_COUNT; i++, effects++) {
        if (effects->type == TWINMOLD_EFFECT_NONE) {
            effects->type = TWINMOLD_EFFECT_FRAGMENT;
            effects->pos = *pos;
            effects->timer = Rand_ZeroFloat(20.0f);
            effects->velocity.x = Rand_CenteredFloat(50.0f);
            effects->velocity.y = Rand_CenteredFloat(50.0f);
            effects->velocity.z = Rand_CenteredFloat(50.0f);
            effects->accel.z = 0.0f;
            effects->accel.x = 0.0f;
            effects->accel.y = -1.5f;
            effects->scale = Rand_ZeroFloat(0.04f) + 0.02f;
            effects->rotY = Rand_ZeroFloat(32767.0f);
            effects->rotX = Rand_ZeroFloat(32767.0f);
            effects->epoch++;
            break;
        }
    }
}

void Boss02_SpawnEffectFlash(TwinmoldEffect* effects, Vec3f* pos) {
    s16 i;

    for (i = 0; i < TWINMOLD_EFFECT_COUNT; i++, effects++) {
        if ((effects->type == TWINMOLD_EFFECT_NONE) || (effects->type == TWINMOLD_EFFECT_FRAGMENT)) {
            effects->type = TWINMOLD_EFFECT_FLASH;
            effects->pos = *pos;
            Math_Vec3f_Copy(&effects->velocity, &gZeroVec3f);
            Math_Vec3f_Copy(&effects->accel, &gZeroVec3f);
            effects->alpha = 255;
            effects->scale = 0.0f;
            effects->epoch++;
            break;
        }
    }
}

void func_809DA50C(s32 arg0, ColliderJntSph* collider, Vec3f* arg2) {
    collider->elements[arg0].dim.worldSphere.center.x = arg2->x;
    collider->elements[arg0].dim.worldSphere.center.y = arg2->y;
    collider->elements[arg0].dim.worldSphere.center.z = arg2->z;
    collider->elements[arg0].dim.worldSphere.radius =
        collider->elements[arg0].dim.modelSphere.radius * collider->elements[arg0].dim.scale * sGiantModeScaleFactor;
}

void Boss02_Init(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;
    s32 i;
    s32 pad[2];

    if (TWINMOLD_GET_TYPE(&this->actor) == BO_TM_SHOT_PARAMS) {
        // Boss Overhaul: this instance is one of the twin dragons' projectiles
        Boss02_BoShotInit(this, play);
        return;
    }

    if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE) &&
        (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED)) {
        sBlueWarp = (DoorWarp1*)Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_DOOR_WARP1, 0.0f, 60.0f,
                                                   0.0f, 0, 0, 0, ENDOORWARP1_FF_1);
        Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_B_HEART, 0.0f, 30.0f, -150.0f, 0, 1, 0, BHEART_PARAM_NORMAL);
    }

    this->actor.attentionRangeType = ATTENTION_RANGE_10;
    this->subCamUp.z = this->subCamUp.x = 0.0f;
    this->subCamUp.y = 1.0f;
    if (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_BATTLE_HANDLER) {
        sTwinmoldBattleHandler = this;
        play->specialEffects = (void*)sTwinmoldEffects;
        this->actor.update = Boss02_BattleHandler_Update;
        this->actor.draw = Boss02_BattleHandler_Draw;
        this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        this->playerScale = 0.01f;
        if ((KREG(64) != 0) || CHECK_EVENTINF(EVENTINF_INTRO_CS_WATCHED_TWINMOLD) || (sBlueWarp != NULL)) {
            this->unk_1D20 = 0;
            sTwinmoldMusicStartTimer = KREG(15) + 20;
        } else {
            this->unk_1D20 = 1;
        }
        R_MAGIC_CONSUME_TIMER_GIANTS_MASK = KREG(14) + 20;
        this->giantModeScaleFactor = 1.0f;
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_EN_TANRON5, 0.0f, 1000.0f, 0.0f, 0, 0, 0,
                           TWINMOLD_PROP_PARAMS(TWINMOLD_PROP_TYPE_SPAWN_HANDLER));
    } else if (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_TAIL) {
        this->actor.update = Boss02_Tail_Update;
        this->actor.draw = NULL;
        this->actor.hintId = TATL_HINT_ID_TWINMOLD;
    } else {
        if (TWINMOLD_GET_TYPE(&this->actor) != TWINMOLD_TYPE_BLUE) {
            this->actor.params = TWINMOLD_TYPE_RED;
            Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_02, 0.0f, 0.0f, 0.0f, 0, 0, 0, TWINMOLD_TYPE_BATTLE_HANDLER);
            sRedTwinmold = this;
            sBlueTwinmold =
                (Boss02*)Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_02, this->actor.world.pos.x,
                                     this->actor.world.pos.y, this->actor.world.pos.z, this->actor.world.rot.x,
                                     this->actor.world.rot.y, this->actor.world.rot.z, TWINMOLD_TYPE_BLUE);
            sRedTwinmold->actor.colChkInfo.damageTable = &sRedTwinmoldDamageTable;
            sBlueTwinmold->actor.colChkInfo.damageTable = &sBlueTwinmoldDamageTable;
            sRedTwinmold->otherTwinmold = sBlueTwinmold;
            sBlueTwinmold->otherTwinmold = sRedTwinmold;
        }

        this->actor.colChkInfo.mass = MASS_HEAVY;
        this->actor.colChkInfo.health = BO_HP(20);
        // Boss Overhaul: the red head is a fire dragon, the blue one an ice dragon. Their attacks are staggered so they
        // don't open fire together at the start.
        this->boBreathElement = (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ? BO_TM_FIRE : BO_TM_ICE;
        this->boBreathCooldown = (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ? 150 : 280;
        this->boRainCooldown = (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ? 380 : 560;
        this->boSpeedScale = 1.0f;
        this->boLastGiantMode = sIsInGiantMode;
        this->boMouthPos = this->actor.world.pos;
        Lights_PointNoGlowSetInfo(&this->boLightInfo, this->actor.world.pos.x, this->actor.world.pos.y,
                                  this->actor.world.pos.z, 0, 0, 0, 0);
        this->boLightNode = LightContext_InsertLight(play, &play->lightCtx, &this->boLightInfo);
        Actor_SetScale(&this->actor, 0.01f);
        SkelAnime_Init(play, &this->skelAnime, &gTwinmoldHeadSkel, &gTwinmoldHeadFlyAnim, this->jointTable,
                       this->morphTable, TWINMOLD_HEAD_LIMB_MAX);
        Collider_InitAndSetJntSph(play, &this->colliderSphere1, &this->actor, &sJntSphInit1,
                                  this->colliderSphere1Elements);
        Collider_InitAndSetJntSph(play, &this->colliderSphere2, &this->actor, &sJntSphInit2,
                                  this->colliderSphere2Elements);
        Collider_InitAndSetCylinder(play, &this->colliderCylinder, &this->actor, &sCylinderInit);

        if (sBlueWarp != NULL) {
            func_809DAA74(this, play);
        } else {
            func_809DAAA8(this, play);
        }

        for (i = 0; i < ARRAY_COUNT(this->unk_01BC); i++) {
            this->unk_01BC[i].x = this->actor.world.pos.x;
            this->unk_01BC[i].y = this->actor.world.pos.y;
            this->unk_01BC[i].z = this->actor.world.pos.z;
        }

        this->unk_014C = Rand_ZeroFloat(1000.0f);
        this->unk_1678 = 22;
        Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_BOSS_02, 0.0f, 0.0f, 0.0f, 0, 0, 0,
                           TWINMOLD_TYPE_TAIL);
    }
}

#define BO_TM_SHOT_MAX 64
#define BO_TM_PATCH_MAX 8

static s16 sBoTmShotCount = 0;
static s16 sBoTmPatchCount = 0;

// [element]: outer glow, its env color, hot core, its env color, arena light
static Color_RGB8 sBoTmGlowPrim[] = { { 255, 200, 80 }, { 215, 245, 255 } };
static Color_RGB8 sBoTmGlowEnv[] = { { 255, 50, 0 }, { 30, 130, 255 } };
static Color_RGB8 sBoTmCorePrim[] = { { 255, 255, 220 }, { 255, 255, 255 } };
static Color_RGB8 sBoTmCoreEnv[] = { { 255, 170, 40 }, { 150, 220, 255 } };
static Color_RGB8 sBoTmLight[] = { { 255, 110, 30 }, { 90, 170, 255 } };

static Color_RGBA8 sBoTmIcePrim = { 170, 255, 255, 255 };
static Color_RGBA8 sBoTmIceEnv = { 0, 50, 100, 255 };

void Boss02_Destroy(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;

    if (TWINMOLD_GET_TYPE(&this->actor) == BO_TM_SHOT_PARAMS) {
        Collider_DestroyCylinder(play, &this->boShot.collider);
        if (sBoTmShotCount > 0) {
            sBoTmShotCount--;
        }
        if ((this->boShot.kind == BO_TM_SHOT_PATCH) && (sBoTmPatchCount > 0)) {
            sBoTmPatchCount--;
        }
    } else if ((TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ||
               (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_BLUE)) {
        LightContext_RemoveLight(play, &play->lightCtx, this->boLightNode);
        this->boLightNode = NULL;
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Boss Overhaul: the twin dragons
 *
 * Red Twinmold is a fire dragon and Blue Twinmold an ice dragon (as in the original game, the red one is weak to ice
 * arrows and the blue one to fire arrows). Each of them:
 *  - Dragon breath: slows down and gathers its element in its jaws (they glow and light up the arena), then pours out
 *    a long stream of fireballs / frost balls that sweeps after Link with a lag. Fire burns, frost freezes. The stream
 *    bursts on the floor, the walls and the ruins (hide behind one: the breath doesn't wear them down).
 *  - Meteor storm (red) / hailstorm (blue): while cruising high above the arena it spits balls into the sky, which come
 *    back down as meteors / giant icicles on marked spots around Link. They smash ruins.
 *  - Eruption: whenever it bursts through the sand, a ring of fire / ice is thrown out of the breach.
 *  - The big attacks leave burning ground (red) or ice spikes (blue) behind for a few seconds.
 *  - Its body burns / sheds frost as it flies.
 * The survivor takes its fallen twin's element too: its breath splits into three jets (its own element in the middle,
 * its twin's on the sides), alternating each time, and its storms mix meteors and icicles.
 *
 * Giant's Mask: the arena and Twinmold shrink while Link wears it, but the attacks shrink less and travel faster
 * relative to the arena, so they stay a threat to giant Link, and they deal their full damage (giant Link normally only
 * takes a quarter). Twinmold also hunts giant Link more often, attacks more often, and its body hurts twice as much.
 * --------------------------------------------------------------------------------------------------------------- */

// Size and speed of the dragons' attacks. In the Giant's Mask arena everything else shrinks by 10, these only by 3.
f32 Boss02_BoFx(void) {
    return sIsInGiantMode ? 0.3f : 1.0f;
}

// Giant Link only takes a quarter of the damage he is dealt: the dragons' attacks make up for it.
u8 Boss02_BoDamage(s32 damage) {
    if (sIsInGiantMode) {
        damage *= 4;
    }
    return CLAMP_MAX(damage, 0xFF);
}

s32 Boss02_BoOwnElement(Boss02* this) {
    return (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ? BO_TM_FIRE : BO_TM_ICE;
}

// The height of the floor under `pos`, or BGCHECK_Y_MIN outside the arena.
f32 Boss02_BoFloorY(PlayState* play, Vec3f* pos) {
    CollisionPoly* poly;
    Vec3f probe = *pos;

    probe.y = sIsInGiantMode ? 5000.0f : 2000.0f;
    return BgCheck_EntityRaycastFloor1(&play->colCtx, &poly, &probe);
}

// True during a cutscene: the intro, a twin's death, the Giant's Mask transformation. (Drinking a potion or changing
// masks doesn't count: the dragons don't wait for Link.)
s32 Boss02_BoInCutscene(PlayState* play) {
    return (play->csCtx.state != CS_STATE_IDLE) || (sTwinmoldBattleHandler == NULL) ||
           (sTwinmoldBattleHandler->unk_1D20 != 0) ||
           (sTwinmoldBattleHandler->giantsMaskCsState != GIANTS_MASK_CS_STATE_WAITING_FOR_MASK);
}

// True while the fight is on and this head is alive and out of its intro.
s32 Boss02_BoCanAttack(Boss02* this, PlayState* play) {
    return BO_ATTACKS_ENABLED() && (this->unk_0144 < 10) && !Boss02_BoInCutscene(play);
}

// How likely a head that just rose out of the sand is to chase Link (0.3 in the original game).
f32 Boss02_BoChaseChance(Boss02* this) {
    if (!BO_ATTACKS_ENABLED()) {
        return 0.3f;
    }
    if (sIsInGiantMode) {
        return 0.6f;
    }
    return (this->unk_0195 != 0) ? 0.45f : 0.3f;
}

// Flying speed multiplier (the dragon slows down while it breathes).
f32 Boss02_BoSpeedScale(Boss02* this) {
    return (this->boSpeedScale > 0.0f) ? this->boSpeedScale : 1.0f;
}

// True when `pos` is inside one of the arena's ruins (a pillar or a pyramid).
s32 Boss02_BoTouchesRuin(PlayState* play, Vec3f* pos, f32 radius) {
    Actor* actor = play->actorCtx.actorLists[ACTORCAT_BOSS].first;

    for (; actor != NULL; actor = actor->next) {
        EnTanron5* ruin = (EnTanron5*)actor;
        f32 dx;
        f32 dz;
        f32 reach;

        // (a ruin hit three times is gone: it only lingers, invisible, for a few frames)
        if ((actor->id != ACTOR_EN_TANRON5) || (actor->update == NULL) ||
            (TWINMOLD_PROP_GET_TYPE(actor) < TWINMOLD_PROP_TYPE_RUIN_PILLAR_1) ||
            (TWINMOLD_PROP_GET_TYPE(actor) > TWINMOLD_PROP_TYPE_RUIN_PYRAMID_8) || (ruin->hitCount >= 3)) {
            continue;
        }

        dx = pos->x - ruin->collider.dim.pos.x;
        dz = pos->z - ruin->collider.dim.pos.z;
        reach = ruin->collider.dim.radius + radius;

        if (((SQ(dx) + SQ(dz)) < SQ(reach)) && (pos->y > (ruin->collider.dim.pos.y - radius)) &&
            (pos->y < (ruin->collider.dim.pos.y + ruin->collider.dim.height + radius))) {
            return true;
        }
    }

    return false;
}

// A burst of fire or ice where a ball hits something. `scale` includes Boss02_BoFx. Only "big" bursts (with a sound)
// leave anything behind: the breath's balls mostly just vanish, there are a lot of them.
void Boss02_BoBurst(PlayState* play, Vec3f* pos, s32 element, f32 scale, s32 big) {
    static Vec3f sFlameAccel = { 0.0f, 0.3f, 0.0f };

    if (!big) {
        return;
    }

    if (element == BO_TM_FIRE) {
        EffectSsKFire_Spawn(play, pos, &gZeroVec3f, &sFlameAccel, (s16)(100.0f * scale), 100);
        SoundSource_PlaySfxAtFixedWorldPos(play, pos, 20, NA_SE_EV_FLAME_IGNITION);
    } else {
        Vec3f velocity;

        velocity.x = 0.0f;
        velocity.y = 1.0f * scale;
        velocity.z = 0.0f;
        EffectSsIceSmoke_Spawn(play, pos, &velocity, &gZeroVec3f, (s16)(800.0f * scale));
        velocity.x = Rand_CenteredFloat(12.0f * scale);
        velocity.y = (Rand_ZeroFloat(6.0f) + 6.0f) * scale;
        velocity.z = Rand_CenteredFloat(12.0f * scale);
        EffectSsEnIce_Spawn(play, pos, 1.6f * scale, &velocity, &gZeroVec3f, &sBoTmIcePrim, &sBoTmIceEnv, 22);
        SoundSource_PlaySfxAtFixedWorldPos(play, pos, 20, NA_SE_EV_ICE_BROKEN);
    }
}

/**
 * Spawns one of the dragons' projectiles, set up for its kind. Returns NULL when there are already too many.
 */
Boss02* Boss02_BoSpawnShot(PlayState* play, s32 kind, s32 element, Vec3f* pos, Vec3f* velocity) {
    Boss02* shotActor;
    Boss02BoShot* shot;
    f32 fx = Boss02_BoFx();
    s32 damage = 0x10;

    if ((sBoTmShotCount >= BO_TM_SHOT_MAX) || ((kind == BO_TM_SHOT_PATCH) && (sBoTmPatchCount >= BO_TM_PATCH_MAX))) {
        return NULL;
    }

    // No burning ground against a ruin: it would wear the ruin down
    if ((kind == BO_TM_SHOT_PATCH) && Boss02_BoTouchesRuin(play, pos, 110.0f * fx)) {
        return NULL;
    }

    shotActor = (Boss02*)Actor_Spawn(&play->actorCtx, play, ACTOR_BOSS_02, pos->x, pos->y, pos->z, 0, 0, 0,
                                     BO_TM_SHOT_PARAMS);
    if (shotActor == NULL) {
        return NULL;
    }

    shot = &shotActor->boShot;
    shot->kind = kind;
    shot->element = element;
    shotActor->actor.velocity = *velocity;

    switch (kind) {
        case BO_TM_SHOT_BREATH:
            shot->size = 0.45f;
            shot->life = 44;
            shot->hitRadius = 60.0f * fx;
            break;

        case BO_TM_SHOT_LOB:
            shot->size = 0.6f;
            shot->life = 90;
            shot->hitRadius = 55.0f * fx;
            break;

        case BO_TM_SHOT_FALLING:
            shot->size = 1.0f;
            shot->hitRadius = 180.0f * fx; // the blast
            shot->target = *pos;
            shot->leavesPatch = true;
            damage = 0x18;
            break;

        case BO_TM_SHOT_PATCH:
            shot->size = 0.0f;
            shot->life = 120;
            shot->hitRadius = 110.0f * fx;
            sBoTmPatchCount++;
            damage = (element == BO_TM_FIRE) ? 0x08 : 0x04;
            // A pool of fire shouldn't restart right under Link's feet if he was just burned by what made it
            shot->hitCooldown = 12;
            SoundSource_PlaySfxAtFixedWorldPos(play, pos, 30,
                                               (element == BO_TM_FIRE) ? NA_SE_EV_FIRE_PILLAR_S : NA_SE_EV_ICE_FREEZE);
            break;

        default: // BO_TM_SHOT_SPARK
            shot->size = 0.5f;
            shot->life = 20;
            shot->hitRadius = 0.0f;
            break;
    }

    // Fire burns, ice freezes. Neither can be blocked with a shield.
    shotActor->boShot.collider.elem.atDmgInfo.effect = (element == BO_TM_FIRE) ? 1 : 2;
    shotActor->boShot.collider.elem.atDmgInfo.damage = Boss02_BoDamage(damage);

    return shotActor;
}

static ColliderCylinderInit sBoTmShotCylinderInit = {
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
        { DMG_UNBLOCKABLE, 0x01, 0x10 },
        { 0x00000000, 0x00, 0x00 },
        ATELEM_ON | ATELEM_SFX_NORMAL,
        ACELEM_NONE,
        OCELEM_NONE,
    },
    { 60, 120, -60, { 0, 0, 0 } },
};

void Boss02_BoShotInit(Boss02* this, PlayState* play) {
    Boss02BoShot* shot = &this->boShot;

    this->actor.flags &= ~(ACTOR_FLAG_ATTENTION_ENABLED | ACTOR_FLAG_HOSTILE);
    this->actor.colChkInfo.health = 0;
    this->actor.gravity = 0.0f;
    this->actor.velocity = gZeroVec3f;
    this->actor.room = -1;
    Actor_SetScale(&this->actor, 0.01f);
    // Out of the boss list: Twinmold and the Giant's Mask transformation go through it
    Actor_ChangeCategory(play, &play->actorCtx, &this->actor, ACTORCAT_MISC);

    Collider_InitAndSetCylinder(play, &shot->collider, &this->actor, &sBoTmShotCylinderInit);
    shot->kind = BO_TM_SHOT_SPARK;
    shot->life = 1;
    shot->spin = (s16)Rand_Next();
    shot->giant = sIsInGiantMode;
    sBoTmShotCount++;

    this->actor.update = Boss02_BoShotUpdate;
    this->actor.draw = Boss02_BoShotDraw;
}

void Boss02_BoShotSetCollider(Boss02* this, f32 radius, f32 height, f32 yShift) {
    Boss02BoShot* shot = &this->boShot;

    shot->collider.dim.radius = radius;
    shot->collider.dim.height = height;
    shot->collider.dim.yShift = yShift;
    Collider_UpdateCylinder(&this->actor, &shot->collider);
}

// Breath balls, lobs and sparks: they fly until they hit something.
void Boss02_BoShotUpdateBall(Boss02* this, PlayState* play) {
    Boss02BoShot* shot = &this->boShot;
    f32 fx = Boss02_BoFx();
    Vec3f prevPos = this->actor.world.pos;
    Vec3f hitPos;
    CollisionPoly* poly = NULL;
    s32 bgId;
    s32 bigBurst;

    // Hit Link last frame
    if (shot->collider.base.atFlags & AT_HIT) {
        shot->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);
        Boss02_BoBurst(play, &this->actor.world.pos, shot->element, fx, true);
        Actor_Kill(&this->actor);
        return;
    }

    // The breath's balls burst small and silently, but one in four (there are a lot of them)
    bigBurst = (shot->kind != BO_TM_SHOT_BREATH) || ((shot->spin & 3) == 0);

    this->actor.velocity.y += shot->gravity;
    this->actor.world.pos.x += this->actor.velocity.x;
    this->actor.world.pos.y += this->actor.velocity.y;
    this->actor.world.pos.z += this->actor.velocity.z;
    Math_ApproachF(&shot->size, 1.0f, 1.0f, 0.1f);
    shot->spin += 0x1800;

    if (shot->life > 0) {
        shot->life--;
    }

    if (shot->life == 0) {
        if (shot->kind == BO_TM_SHOT_SPARK) {
            Boss02_BoBurst(play, &this->actor.world.pos, shot->element, 0.6f * fx, false);
        }
        Actor_Kill(&this->actor);
        return;
    }

    if (shot->kind == BO_TM_SHOT_SPARK) {
        return; // only for show
    }

    // The ruins stop it like walls do, without being worn down by it (it bursts before its hitbox can touch one)
    if (Boss02_BoTouchesRuin(play, &this->actor.world.pos, shot->hitRadius * shot->size)) {
        Boss02_BoBurst(play, &this->actor.world.pos, shot->element, fx, bigBurst);
        Actor_Kill(&this->actor);
        return;
    }

    if (BgCheck_EntityLineTest1(&play->colCtx, &prevPos, &this->actor.world.pos, &hitPos, &poly, true, true, true, true,
                                &bgId)) {
        this->actor.world.pos = hitPos;
        Boss02_BoBurst(play, &hitPos, shot->element, fx, bigBurst);

        // Landed on the floor rather than a wall: the ground burns / freezes
        if (shot->leavesPatch && (poly != NULL) && (COLPOLY_GET_NORMAL(poly->normal.y) > 0.5f)) {
            Boss02_BoSpawnShot(play, BO_TM_SHOT_PATCH, shot->element, &hitPos, &gZeroVec3f);
        }

        Actor_Kill(&this->actor);
        return;
    }

    Boss02_BoShotSetCollider(this, shot->hitRadius * shot->size, shot->hitRadius * shot->size * 2.0f,
                             -shot->hitRadius * shot->size);
    CollisionCheck_SetAT(play, &play->colChkCtx, &shot->collider.base);
}

// Meteors and icicles: wait for their turn, mark the spot, drop, blast.
void Boss02_BoShotUpdateFalling(Boss02* this, PlayState* play) {
    static Vec3f sFlameAccel = { 0.0f, 0.3f, 0.0f };
    Boss02BoShot* shot = &this->boShot;
    f32 fx = Boss02_BoFx();
    f32 height = 1500.0f * fx;
    Vec3f pos;

    shot->spin += 0x1000;

    switch (shot->phase) {
        case 0: // waiting for its turn
            this->actor.world.pos = shot->target;
            this->actor.world.pos.y += height;
            if (--shot->life <= 0) {
                shot->phase = 1;
                shot->life = 36;
            }
            break;

        case 1: // the landing spot is marked
            if (--shot->life <= 0) {
                shot->phase = 2;
                shot->life = 12;
                SoundSource_PlaySfxAtFixedWorldPos(
                    play, &shot->target, 20, (shot->element == BO_TM_FIRE) ? NA_SE_EV_COMING_FIRE : NA_SE_EV_ICE_PILLAR_FALL);
            }
            break;

        case 2: // falling
            shot->life--;
            this->actor.world.pos = shot->target;
            this->actor.world.pos.y += (height * shot->life) / 12.0f;

            if ((shot->timer % 8) == 0) {
                pos = this->actor.world.pos;
                if (shot->element == BO_TM_FIRE) {
                    EffectSsKFire_Spawn(play, &pos, &gZeroVec3f, &sFlameAccel, (s16)(130.0f * fx), 100);
                } else {
                    EffectSsIceSmoke_Spawn(play, &pos, &gZeroVec3f, &gZeroVec3f, (s16)(1000.0f * fx));
                }
            }

            if (shot->life <= 0) {
                Vec3f zero = gZeroVec3f;
                Color_RGBA8 ringPrim;
                Color_RGBA8 ringEnv;
                s32 element = shot->element;

                shot->phase = 3;
                shot->life = 4;
                this->actor.world.pos = shot->target;
                pos = shot->target;
                pos.y += 30.0f * fx;

                ringPrim.r = sBoTmGlowPrim[element].r;
                ringPrim.g = sBoTmGlowPrim[element].g;
                ringPrim.b = sBoTmGlowPrim[element].b;
                ringPrim.a = 255;
                ringEnv.r = sBoTmGlowEnv[element].r;
                ringEnv.g = sBoTmGlowEnv[element].g;
                ringEnv.b = sBoTmGlowEnv[element].b;
                ringEnv.a = 0;
                EffectSsBlast_Spawn(play, &shot->target, &zero, &zero, &ringPrim, &ringEnv, (s16)(300.0f * fx),
                                    (s16)(900.0f * fx), (s16)(60.0f * fx), 12);

                if (element == BO_TM_FIRE) {
                    EffectSsBomb2_SpawnLayered(play, &pos, &zero, &zero, (s16)(160.0f * fx), (s16)(25.0f * fx));
                    SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 40, NA_SE_IT_BOMB_EXPLOSION);
                } else {
                    Boss02_BoBurst(play, &pos, BO_TM_ICE, 1.4f * fx, true);
                    SoundSource_PlaySfxAtFixedWorldPos(play, &pos, 40, NA_SE_IT_EXPLOSION_ICE);
                }
                Actor_RequestQuakeAndRumble(&this->actor, play, 3, 6);
            }
            break;

        default: // the blast
            // (it never registers its hitbox on the frame it dies: whatever it hits reads it on the next frame)
            if (shot->life <= 0) {
                if (shot->leavesPatch) {
                    Boss02_BoSpawnShot(play, BO_TM_SHOT_PATCH, shot->element, &shot->target, &gZeroVec3f);
                }
                Actor_Kill(&this->actor);
                return;
            }
            shot->life--;
            Boss02_BoShotSetCollider(this, shot->hitRadius, 150.0f * fx, 0.0f);
            CollisionCheck_SetAT(play, &play->colChkCtx, &shot->collider.base);
            return;
    }

    // The falling ball itself
    if (shot->phase == 2) {
        Boss02_BoShotSetCollider(this, 90.0f * fx, 180.0f * fx, -90.0f * fx);
        CollisionCheck_SetAT(play, &play->colChkCtx, &shot->collider.base);
    }
}

// Burning ground / ice spikes.
void Boss02_BoShotUpdatePatch(Boss02* this, PlayState* play) {
    Boss02BoShot* shot = &this->boShot;
    f32 fx = Boss02_BoFx();

    shot->spin += 0x300;
    Math_ApproachF(&shot->size, 1.0f, 1.0f, 0.15f);

    if (--shot->life <= 0) {
        Actor_Kill(&this->actor);
        return;
    }

    if (shot->hitCooldown > 0) {
        shot->hitCooldown--;
    }

    if (shot->collider.base.atFlags & AT_HIT) {
        shot->collider.base.atFlags &= ~(AT_HIT | AT_BOUNCED);

        if (shot->element == BO_TM_ICE) {
            // The spikes that froze Link shatter (it stops registering its hitbox the frame before it dies)
            Boss02_BoBurst(play, &this->actor.world.pos, BO_TM_ICE, fx, true);
            Actor_Kill(&this->actor);
            return;
        }
        shot->hitCooldown = 40;
    }

    if ((shot->hitCooldown == 0) && (shot->size > 0.6f) && (shot->life > 10)) {
        Boss02_BoShotSetCollider(this, shot->hitRadius * shot->size, ((shot->element == BO_TM_FIRE) ? 70.0f : 50.0f) * fx,
                                 0.0f);
        CollisionCheck_SetAT(play, &play->colChkCtx, &shot->collider.base);
    }
}

void Boss02_BoShotUpdate(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;
    Boss02BoShot* shot = &this->boShot;

    shot->timer++;

    // The arena changed size, or a cutscene started: the attack is gone
    if ((shot->giant != sIsInGiantMode) || Boss02_BoInCutscene(play)) {
        Actor_Kill(&this->actor);
        return;
    }

    switch (shot->kind) {
        case BO_TM_SHOT_FALLING:
            Boss02_BoShotUpdateFalling(this, play);
            break;

        case BO_TM_SHOT_PATCH:
            Boss02_BoShotUpdatePatch(this, play);
            break;

        default:
            Boss02_BoShotUpdateBall(this, play);
            break;
    }
}

/* Drawing: everything is gameplay_keep (light orb, flame, ice shard, ice smoke, shockwave ring) */

void Boss02_BoDrawGlow(PlayState* play, Vec3f* pos, f32 scale, s16 spin, Color_RGB8* prim, Color_RGB8* env, u8 alpha) {
    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, prim->r, prim->g, prim->b, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, env->r, env->g, env->b, 128);
    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_Scale(scale, scale, 0.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_RotateZS(spin, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);
    gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

// A tongue of flame (the fire of Effect_Ss_K_Fire), turned by `rotZ` around the view axis.
void Boss02_BoDrawFlame(PlayState* play, Vec3f* pos, f32 xzScale, f32 yScale, s16 rotZ, u8 alpha) {
    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_Scale(xzScale, yScale, xzScale, MTXMODE_APPLY);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 0x20, 0x40, 1, 0, play->state.frames * -20, 0x20, 0x80, 0,
                                  0, 0, -20));
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 0, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 10, 0, 0);
    gDPPipeSync(POLY_XLU_DISP++);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_RotateZS(rotZ, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, gEffFire1DL);

    CLOSE_DISPS(play->state.gfxCtx);
}

// An ice crystal (the shard of Effect_Ss_En_Ice).
void Boss02_BoDrawCrystal(PlayState* play, Vec3f* pos, f32 scale, s16 yaw, s16 pitch, u8 alpha) {
    u32 frames = play->gameplayFrames;
    Vec3f hilitePos = *pos;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    Matrix_RotateYS(yaw, MTXMODE_APPLY);
    Matrix_RotateXS(pitch, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    func_800BCC68(&hilitePos, play);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, frames & 0xFF, 0x20, 0x10, 1, 0, (frames * 2) & 0xFF, 0x40,
                                  0x20, 0, 1, 0, 2));
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 170, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 50, 100, alpha);
    gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment2MaterialDL);
    gSPDisplayList(POLY_XLU_DISP++, gEffIceFragment2ModelDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

// A flat ring on the ground (the shockwave of Effect_Ss_Blast), `radius` in world units.
void Boss02_BoDrawRing(PlayState* play, Vec3f* pos, f32 radius, s32 element, u8 alpha) {
    f32 scale = radius * 0.1f;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gDPSetEnvColor(POLY_XLU_DISP++, sBoTmGlowEnv[element].r, sBoTmGlowEnv[element].g, sBoTmGlowEnv[element].b, 0);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, sBoTmGlowPrim[element].r, sBoTmGlowPrim[element].g, sBoTmGlowPrim[element].b,
                    alpha);
    Matrix_Translate(pos->x, pos->y + (4.0f * Boss02_BoFx()), pos->z, MTXMODE_NEW);
    Matrix_Scale(scale, 1.0f, scale, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, gEffShockwaveDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * A fireball (a white-hot core in a ball of swirling flames) or a frost ball (an ice crystal in a cloud of mist).
 * `size` is 1 for a breath ball.
 */
void Boss02_BoDrawBall(PlayState* play, Vec3f* pos, s32 element, f32 size, s16 spin, u8 alpha) {
    // Kept light (a glow and a flame / a crystal): dozens of these can be in the air at once
    Boss02_BoDrawGlow(play, pos, 22.0f * size, spin, &sBoTmGlowPrim[element], &sBoTmGlowEnv[element], (u8)(alpha * 0.7f));

    if (element == BO_TM_FIRE) {
        Boss02_BoDrawFlame(play, pos, 0.012f * size, 0.012f * size, spin, alpha);
        Boss02_BoDrawGlow(play, pos, 9.0f * size, -spin, &sBoTmCorePrim[element], &sBoTmCoreEnv[element], alpha);
    } else {
        Boss02_BoDrawCrystal(play, pos, 4.0f * size, spin, spin * 2, alpha);
    }
}

void Boss02_BoShotDraw(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;
    Boss02BoShot* shot = &this->boShot;
    f32 fx = Boss02_BoFx();
    Vec3f pos;
    s32 i;
    u8 alpha;

    switch (shot->kind) {
        case BO_TM_SHOT_FALLING:
            if (shot->phase == 1) {
                // A ring closing in on the spot, and the blast's reach
                f32 closing = (f32)shot->life / 36.0f;
                u8 ringAlpha = 150 + (s32)(105.0f * fabsf(Math_SinS(shot->timer * 0x1800)));

                Boss02_BoDrawRing(play, &shot->target, shot->hitRadius * (1.0f + (1.5f * closing)), shot->element,
                                  ringAlpha);
                Boss02_BoDrawRing(play, &shot->target, shot->hitRadius, shot->element, 120);
            } else if (shot->phase == 2) {
                Boss02_BoDrawRing(play, &shot->target, shot->hitRadius, shot->element, 255);
                if (shot->element == BO_TM_FIRE) {
                    Boss02_BoDrawBall(play, &this->actor.world.pos, BO_TM_FIRE, 2.0f * fx, shot->spin, 255);
                } else {
                    // A giant icicle, point down, in a cold glow
                    Boss02_BoDrawGlow(play, &this->actor.world.pos, 35.0f * fx, shot->spin,
                                      &sBoTmGlowPrim[BO_TM_ICE], &sBoTmGlowEnv[BO_TM_ICE], 150);
                    Boss02_BoDrawCrystal(play, &this->actor.world.pos, 12.0f * fx, shot->spin, 0x4000, 255);
                }
            }
            break;

        case BO_TM_SHOT_PATCH:
            alpha = (shot->life < 20) ? (shot->life * 12) : 240;
            pos = this->actor.world.pos;
            pos.y += 15.0f * fx;
            Boss02_BoDrawGlow(play, &pos, 40.0f * fx * shot->size, shot->spin, &sBoTmGlowPrim[shot->element],
                              &sBoTmGlowEnv[shot->element], alpha / 3);

            // One in the middle, two around it
            for (i = 0; i < 3; i++) {
                s16 angle = shot->spin + (i * 0x8000);
                f32 dist = (i == 0) ? 0.0f : (shot->hitRadius * 0.6f * shot->size);

                pos.x = this->actor.world.pos.x + (Math_SinS(angle) * dist);
                pos.y = this->actor.world.pos.y;
                pos.z = this->actor.world.pos.z + (Math_CosS(angle) * dist);

                if (shot->element == BO_TM_FIRE) {
                    f32 flicker = 0.85f + (0.3f * Math_SinS((shot->timer * 0x1300) + (i * 0x2E00)));

                    pos.y += 5.0f * fx;
                    Boss02_BoDrawFlame(play, &pos, 0.009f * fx * shot->size, 0.011f * fx * shot->size * flicker,
                                       (i & 1) ? 0x8000 : 0, alpha);
                } else {
                    pos.y += 25.0f * fx * shot->size;
                    Boss02_BoDrawCrystal(play, &pos, ((i == 0) ? 7.0f : 4.5f) * fx * shot->size, i * 0x5555,
                                         0x4000 + ((i - 1) * 0x900), alpha);
                }
            }
            break;

        default:
            alpha = (shot->life < 6) ? (shot->life * 40) : 255;
            Boss02_BoDrawBall(play, &this->actor.world.pos, shot->element,
                              shot->size * fx * ((shot->kind == BO_TM_SHOT_SPARK) ? 0.8f : 1.0f), shot->spin, alpha);
            break;
    }
}

/* The dragons themselves */

// Twinmold's jaws while it gathers and pours out its breath.
void Boss02_BoDrawMouth(Boss02* this, PlayState* play) {
    f32 fx = Boss02_BoFx();
    f32 glow = this->boGlow;
    s32 element = this->boBreathElement;
    Vec3f pos = this->unk_017C;

    this->boMouthPos = this->unk_017C;

    if (glow < 0.02f) {
        return;
    }

    glow *= 0.9f + (0.1f * Math_SinS(this->unk_014C * 0x2800));
    Boss02_BoDrawGlow(play, &pos, 40.0f * fx * glow, this->unk_014C * 0x300, &sBoTmGlowPrim[element],
                      &sBoTmGlowEnv[element], (u8)(200.0f * this->boGlow));

    if (element == BO_TM_FIRE) {
        Boss02_BoDrawFlame(play, &pos, 0.012f * fx * glow, 0.015f * fx * glow, this->unk_014C * 0x700,
                           (u8)(255.0f * this->boGlow));
    } else {
        Boss02_BoDrawCrystal(play, &pos, 5.0f * fx * glow, this->unk_014C * 0x500, this->unk_014C * 0x300,
                             (u8)(255.0f * this->boGlow));
    }

    Boss02_BoDrawGlow(play, &pos, 16.0f * fx * glow, -this->unk_014C * 0x500, &sBoTmCorePrim[element],
                      &sBoTmCoreEnv[element], (u8)(255.0f * this->boGlow));
}

// In the Giant's Mask arena, touching Twinmold hurts twice as much as in the original game.
void Boss02_BoSetContactDamage(Boss02* this) {
    u8 damage = (BO_ATTACKS_ENABLED() && sIsInGiantMode) ? 0x20 : 0x10;
    s32 i;

    for (i = 0; i < ARRAY_COUNT(this->colliderSphere1Elements); i++) {
        this->colliderSphere1Elements[i].base.atDmgInfo.damage = damage;
    }
    for (i = 0; i < ARRAY_COUNT(this->colliderSphere2Elements); i++) {
        this->colliderSphere2Elements[i].base.atDmgInfo.damage = damage;
    }
}

// The jaws light up the arena in the breath's color.
void Boss02_BoUpdateLight(Boss02* this) {
    s32 element = this->boBreathElement;
    f32 glow = this->boGlow;

    Lights_PointNoGlowSetInfo(&this->boLightInfo, this->boMouthPos.x, this->boMouthPos.y, this->boMouthPos.z,
                              (u8)(sBoTmLight[element].r * glow), (u8)(sBoTmLight[element].g * glow),
                              (u8)(sBoTmLight[element].b * glow),
                              (s16)(glow * (sIsInGiantMode ? 500.0f : 2200.0f)));
}

// Flames lick off the fire dragon's body; the ice dragon sheds freezing mist.
void Boss02_BoAura(Boss02* this, PlayState* play) {
    static Vec3f sFlameAccel = { 0.0f, 0.3f, 0.0f };
    f32 fx = Boss02_BoFx();
    f32 floorY = sIsInGiantMode ? 3150.0f : 0.0f;
    s32 element = Boss02_BoOwnElement(this);
    Vec3f pos;
    Vec3f velocity;

    if ((this->unk_014C % 16) != 0) {
        return;
    }

    // The survivor carries both elements
    if ((this->unk_0195 != 0) && ((this->unk_014C / 16) % 2)) {
        element ^= 1;
    }

    pos = this->unk_147C[(s32)Rand_ZeroFloat(19.99f) + 1];
    if (pos.y < (floorY + (40.0f * sGiantModeScaleFactor))) {
        return;
    }

    velocity.x = 0.0f;
    velocity.z = 0.0f;
    if (element == BO_TM_FIRE) {
        velocity.y = 1.5f * fx;
        EffectSsKFire_Spawn(play, &pos, &velocity, &sFlameAccel, (s16)(70.0f * fx), 100);
    } else {
        velocity.y = -1.0f * fx;
        EffectSsIceSmoke_Spawn(play, &pos, &velocity, &gZeroVec3f, (s16)(600.0f * fx));
    }
}

// After a breath: rest, and the survivor switches elements.
void Boss02_BoEndBreath(Boss02* this) {
    s32 enraged = (this->unk_0195 != 0);

    this->boBreathCooldown = (s16)(enraged ? (Rand_ZeroFloat(60.0f) + 150.0f) : (Rand_ZeroFloat(80.0f) + 200.0f));
    if (sIsInGiantMode) {
        this->boBreathCooldown = (this->boBreathCooldown * 3) / 4;
    }
    if (enraged) {
        this->boBreathElement ^= 1;
    }
}

/**
 * Dragon breath: gather (the jaws glow brighter and brighter, the dragon slows down), then pour out a stream that sweeps
 * after Link with a lag. The survivor's breath has three jets, alternating which element is in the middle.
 */
void Boss02_BoBreath(Boss02* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 fx = Boss02_BoFx();
    f32 world = sGiantModeScaleFactor;
    s32 enraged = (this->unk_0195 != 0);
    s32 element = this->boBreathElement;
    Vec3f forward;
    Vec3f mouth = this->boMouthPos;
    Vec3f toPlayer;
    Vec3f velocity;
    f32 dist;
    f32 groundY;

    forward.x = Math_SinS(this->actor.world.rot.y) * Math_CosS(this->actor.world.rot.x);
    forward.y = Math_SinS(this->actor.world.rot.x);
    forward.z = Math_CosS(this->actor.world.rot.y) * Math_CosS(this->actor.world.rot.x);

    // Before the first draw (or right after the arena changed size) the jaws aren't known yet
    if (Math_Vec3f_DistXYZ(&mouth, &this->actor.world.pos) > (800.0f * world)) {
        mouth.x = this->actor.world.pos.x + (forward.x * 300.0f * world);
        mouth.y = this->actor.world.pos.y + (forward.y * 300.0f * world);
        mouth.z = this->actor.world.pos.z + (forward.z * 300.0f * world);
    }

    groundY = Boss02_BoFloorY(play, &this->actor.world.pos);
    if (groundY == BGCHECK_Y_MIN) {
        groundY = sIsInGiantMode ? 3150.0f : 0.0f;
    }

    if (this->boWindupTimer > 0) {
        // Gathering: fire / frost is drawn into the jaws
        this->boWindupTimer--;
        Math_ApproachF(&this->boGlow, 1.0f, 1.0f, 0.045f);
        Math_ApproachF(&this->boSpeedScale, 0.45f, 1.0f, 0.04f);

        if ((this->boWindupTimer % 6) == 0) {
            Vec3f from;
            s16 yaw = (s16)Rand_Next();
            s16 pitch = (s16)Rand_CenteredFloat(0x6000);
            f32 reach = 260.0f * fx;

            from.x = mouth.x + (Math_SinS(yaw) * Math_CosS(pitch) * reach);
            from.y = mouth.y + (Math_SinS(pitch) * reach);
            from.z = mouth.z + (Math_CosS(yaw) * Math_CosS(pitch) * reach);
            // They reach the jaws as they fade out (flames last about 38 frames, mist 50), following the head
            velocity.x = ((mouth.x - from.x) / ((element == BO_TM_FIRE) ? 38.0f : 50.0f)) + this->actor.velocity.x;
            velocity.y = ((mouth.y - from.y) / ((element == BO_TM_FIRE) ? 38.0f : 50.0f)) + this->actor.velocity.y;
            velocity.z = ((mouth.z - from.z) / ((element == BO_TM_FIRE) ? 38.0f : 50.0f)) + this->actor.velocity.z;

            if (element == BO_TM_FIRE) {
                EffectSsKFire_Spawn(play, &from, &velocity, &gZeroVec3f, (s16)(50.0f * fx), 100);
            } else {
                EffectSsIceSmoke_Spawn(play, &from, &velocity, &gZeroVec3f, (s16)(500.0f * fx));
            }
        }

        if (this->boWindupTimer == 0) {
            this->boBreathTimer = enraged ? 84 : 60;
            toPlayer.x = player->actor.world.pos.x - mouth.x;
            toPlayer.y = player->actor.world.pos.y - mouth.y;
            toPlayer.z = player->actor.world.pos.z - mouth.z;
            dist = sqrtf(SQ(toPlayer.x) + SQ(toPlayer.y) + SQ(toPlayer.z));
            // The stream starts straight ahead and swings onto Link from there
            this->boBreathTarget.x = mouth.x + (forward.x * dist);
            this->boBreathTarget.y = mouth.y + (forward.y * dist);
            this->boBreathTarget.z = mouth.z + (forward.z * dist);
            Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_ROAR_OLD);
        }
        return;
    }

    if (this->boBreathTimer > 0) {
        Vec3f target;

        this->boBreathTimer--;
        Math_ApproachF(&this->boGlow, 1.0f, 1.0f, 0.1f);
        Math_ApproachF(&this->boSpeedScale, 0.45f, 1.0f, 0.04f);

        // The aim point chases Link at a limited speed
        target.x = player->actor.world.pos.x;
        target.y = player->actor.world.pos.y + (25.0f * fx);
        target.z = player->actor.world.pos.z;
        Math_ApproachF(&this->boBreathTarget.x, target.x, 1.0f, 38.0f * fx);
        Math_ApproachF(&this->boBreathTarget.y, target.y, 1.0f, 38.0f * fx);
        Math_ApproachF(&this->boBreathTarget.z, target.z, 1.0f, 38.0f * fx);

        // It can't breathe backwards, or through the sand it dives into
        toPlayer.x = this->boBreathTarget.x - mouth.x;
        toPlayer.y = this->boBreathTarget.y - mouth.y;
        toPlayer.z = this->boBreathTarget.z - mouth.z;
        if ((((forward.x * toPlayer.x) + (forward.y * toPlayer.y) + (forward.z * toPlayer.z)) < 0.0f) ||
            (mouth.y < (groundY + (120.0f * world)))) {
            this->boBreathTimer = 0;
            Boss02_BoEndBreath(this);
            return;
        }

        Actor_PlaySfx(&this->actor, ((element == BO_TM_FIRE) ? NA_SE_EN_DODO_J_FIRE : NA_SE_EN_FREEZAD_BREATH) - SFX_FLAG);

        if ((this->boBreathTimer % 3) == 0) {
            BoOrb_AimVelocity(&mouth, &this->boBreathTarget, 75.0f * fx, 0, &velocity);
            Boss02_BoSpawnShot(play, BO_TM_SHOT_BREATH, element, &mouth, &velocity);

            if (enraged) {
                // The fallen twin's element on either side
                BoOrb_AimVelocity(&mouth, &this->boBreathTarget, 75.0f * fx, 0x900, &velocity);
                Boss02_BoSpawnShot(play, BO_TM_SHOT_BREATH, element ^ 1, &mouth, &velocity);
                BoOrb_AimVelocity(&mouth, &this->boBreathTarget, 75.0f * fx, -0x900, &velocity);
                Boss02_BoSpawnShot(play, BO_TM_SHOT_BREATH, element ^ 1, &mouth, &velocity);
            }
        } else if (((this->boBreathTimer % 6) == 1) && (!enraged || ((this->boBreathTimer % 12) == 1))) {
            // The breath itself, pouring out of the jaws (half as often for the three jets of the survivor)
            BoOrb_AimVelocity(&mouth, &this->boBreathTarget, 30.0f * fx, (s16)Rand_CenteredFloat(0x600), &velocity);
            if (element == BO_TM_FIRE) {
                EffectSsKFire_Spawn(play, &mouth, &velocity, &gZeroVec3f, (s16)(90.0f * fx), 100);
            } else {
                EffectSsIceSmoke_Spawn(play, &mouth, &velocity, &gZeroVec3f, (s16)(800.0f * fx));
            }
        }

        if (this->boBreathTimer == 0) {
            Boss02_BoEndBreath(this);
        }
        return;
    }

    Math_ApproachZeroF(&this->boGlow, 1.0f, 0.06f);
    Math_ApproachF(&this->boSpeedScale, 1.0f, 1.0f, 0.04f);

    if (this->boBreathCooldown > 0) {
        this->boBreathCooldown--;
        return;
    }

    // Only out of the sand: rising (1), flying across the arena (2) or chasing Link (5)
    if ((this->unk_0144 != 1) && (this->unk_0144 != 2) && (this->unk_0144 != 5)) {
        return;
    }

    if (this->actor.world.pos.y < (groundY + (200.0f * world))) {
        return;
    }

    toPlayer.x = player->actor.world.pos.x - mouth.x;
    toPlayer.y = player->actor.world.pos.y - mouth.y;
    toPlayer.z = player->actor.world.pos.z - mouth.z;
    dist = sqrtf(SQ(toPlayer.x) + SQ(toPlayer.y) + SQ(toPlayer.z));

    if ((dist < (300.0f * world)) || (dist > (3500.0f * world))) {
        return;
    }

    // Link has to be roughly in front of the head
    if (((forward.x * toPlayer.x) + (forward.y * toPlayer.y) + (forward.z * toPlayer.z)) < (0.35f * dist)) {
        return;
    }

    this->boWindupTimer = 28;
    Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_ROAR_OLD);
    Actor_PlaySfx(&this->actor, (element == BO_TM_FIRE) ? NA_SE_EN_DODO_J_BREATH : NA_SE_EV_ICE_PILLAR_RISING);
}

/**
 * Meteor storm (fire) / hailstorm (ice): while cruising high above the arena, the dragon spits balls into the sky and
 * they come back down on marked spots around Link, one after another, the first one where he stood.
 */
void Boss02_BoStorm(Boss02* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    f32 fx = Boss02_BoFx();
    f32 world = sGiantModeScaleFactor;
    s32 enraged = (this->unk_0195 != 0);
    s32 own = Boss02_BoOwnElement(this);
    Vec3f mouth = this->boMouthPos;
    Vec3f velocity;
    Vec3f spot;
    s32 count;
    s32 i;
    f32 floorY;

    if ((this->boWindupTimer > 0) || (this->boBreathTimer > 0)) {
        return;
    }

    if (this->boRainCooldown > 0) {
        this->boRainCooldown--;
        return;
    }

    if ((this->unk_0144 != 2) || (Math_Vec3f_DistXYZ(&mouth, &this->actor.world.pos) > (800.0f * world))) {
        return;
    }

    floorY = Boss02_BoFloorY(play, &this->actor.world.pos);
    if ((floorY == BGCHECK_Y_MIN) || (this->actor.world.pos.y < (floorY + (400.0f * world)))) {
        return;
    }

    // Balls spat into the sky
    for (i = 0; i < 2; i++) {
        velocity.x = Rand_CenteredFloat(16.0f * fx);
        velocity.y = 45.0f * fx;
        velocity.z = Rand_CenteredFloat(16.0f * fx);
        Boss02_BoSpawnShot(play, BO_TM_SHOT_SPARK, (enraged && (i == 1)) ? (own ^ 1) : own, &mouth, &velocity);
    }

    count = enraged ? 7 : 5;
    for (i = 0; i < count; i++) {
        Boss02* falling;
        s32 element = own;

        spot = player->actor.world.pos;
        if (i != 0) {
            s16 yaw = (s16)Rand_Next();
            f32 dist = (Rand_ZeroFloat(380.0f) + 120.0f) * fx;

            spot.x += Math_SinS(yaw) * dist;
            spot.z += Math_CosS(yaw) * dist;
        }

        floorY = Boss02_BoFloorY(play, &spot);
        if (floorY == BGCHECK_Y_MIN) {
            continue; // outside the arena
        }
        spot.y = floorY;

        if (enraged && (((i + this->boRainFlip) % 2) != 0)) {
            element = own ^ 1;
        }

        falling = Boss02_BoSpawnShot(play, BO_TM_SHOT_FALLING, element, &spot, &gZeroVec3f);
        if (falling != NULL) {
            // Wait for the balls to go up, then one after another
            falling->boShot.life = 14 + (i * 7);
        }
    }

    this->boRainFlip ^= 1;
    Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_ROAR_OLD);
    Actor_PlaySfx(&this->actor, (own == BO_TM_FIRE) ? NA_SE_EV_FLAME_IGNITION : NA_SE_EV_ICE_SWING);

    this->boRainCooldown = (s16)(enraged ? (Rand_ZeroFloat(70.0f) + 330.0f) : (Rand_ZeroFloat(90.0f) + 430.0f));
    if (sIsInGiantMode) {
        this->boRainCooldown = (this->boRainCooldown * 3) / 4;
    }
}

/**
 * Called every frame for both heads, after their action.
 */
void Boss02_BoDragonUpdate(Boss02* this, PlayState* play) {
    if (this->boEruptCooldown > 0) {
        this->boEruptCooldown--;
    }

    Boss02_BoSetContactDamage(this);

    // The arena changed size: whatever was in progress is dropped (its positions are no longer valid)
    if (this->boLastGiantMode != sIsInGiantMode) {
        this->boLastGiantMode = sIsInGiantMode;
        this->boWindupTimer = 0;
        this->boBreathTimer = 0;
        this->boMouthPos = this->actor.world.pos;
    }

    if (BO_ATTACKS_ENABLED() &&
        ((this->unk_0144 < 10) || (this->unk_0144 == 11) || (this->unk_0144 == 12))) {
        Boss02_BoAura(this, play);
    }

    if (!Boss02_BoCanAttack(this, play)) {
        this->boWindupTimer = 0;
        this->boBreathTimer = 0;
        Math_ApproachZeroF(&this->boGlow, 1.0f, 0.08f);
        Math_ApproachF(&this->boSpeedScale, 1.0f, 1.0f, 0.05f);
    } else {
        Boss02_BoBreath(this, play);
        Boss02_BoStorm(this, play);
    }

    Boss02_BoUpdateLight(this);
}

/**
 * Whenever Twinmold bursts through the sand, a ring of fire / ice is thrown out of the breach and rains down around it,
 * half of it leaving burning ground / ice spikes where it lands.
 */
void Boss02_BoEruption(Boss02* this, PlayState* play, Vec3f* breachPos) {
    static Vec3f sFlameAccel = { 0.0f, 0.3f, 0.0f };
    f32 fx = Boss02_BoFx();
    s32 own = Boss02_BoOwnElement(this);
    s32 enraged = (this->unk_0195 != 0);
    Vec3f from;
    Vec3f to;
    Vec3f velocity;
    s32 i;
    s32 count;
    s16 baseYaw;

    if (!Boss02_BoCanAttack(this, play) || (this->boEruptCooldown > 0)) {
        return;
    }

    count = enraged ? 8 : 6;
    baseYaw = (s16)Rand_Next();
    from = *breachPos;
    from.y += 40.0f * fx;

    // A pillar of fire / ice out of the breach
    for (i = 0; i < 2; i++) {
        Vec3f pillarPos = from;

        pillarPos.x += Rand_CenteredFloat(120.0f * fx);
        pillarPos.z += Rand_CenteredFloat(120.0f * fx);
        velocity.x = 0.0f;
        velocity.y = (6.0f + (i * 3.0f)) * fx;
        velocity.z = 0.0f;
        if (own == BO_TM_FIRE) {
            EffectSsKFire_Spawn(play, &pillarPos, &velocity, &sFlameAccel, (s16)(170.0f * fx), 100);
        } else {
            EffectSsIceSmoke_Spawn(play, &pillarPos, &velocity, &gZeroVec3f, (s16)(1400.0f * fx));
            if (i == 0) {
                EffectSsEnIce_Spawn(play, &pillarPos, 2.5f * fx, &velocity, &gZeroVec3f, &sBoTmIcePrim, &sBoTmIceEnv,
                                    30);
            }
        }
    }
    SoundSource_PlaySfxAtFixedWorldPos(play, &from, 30,
                                       (own == BO_TM_FIRE) ? NA_SE_EV_FIRE_PILLAR : NA_SE_EV_ICE_PILLAR_RISING);

    for (i = 0; i < count; i++) {
        Boss02* lob;
        f32 radius = (Rand_ZeroFloat(300.0f) + 380.0f) * fx;
        s16 yaw = baseYaw + (i * (0x10000 / count));
        f32 floorY;
        s32 element = (enraged && (i % 2)) ? (own ^ 1) : own;

        to.x = breachPos->x + (Math_SinS(yaw) * radius);
        to.y = breachPos->y;
        to.z = breachPos->z + (Math_CosS(yaw) * radius);
        floorY = Boss02_BoFloorY(play, &to);
        if (floorY != BGCHECK_Y_MIN) {
            to.y = floorY;
        }

        BoOrb_ArcVelocity(&from, &to, 34.0f, -1.3f * fx, &velocity);
        lob = Boss02_BoSpawnShot(play, BO_TM_SHOT_LOB, element, &from, &velocity);
        if (lob != NULL) {
            lob->boShot.gravity = -1.3f * fx;
            lob->boShot.leavesPatch = ((i % 2) == 0);
        }
    }

    this->boEruptCooldown = 150;
}

void func_809DAA74(Boss02* this, PlayState* play) {
    this->actionFunc = func_809DAA98;
    this->actor.world.pos.y = -3000.0f;
}

void func_809DAA98(Boss02* this, PlayState* play) {
}

void func_809DAAA8(Boss02* this, PlayState* play) {
    this->actionFunc = func_809DAB78;
    Animation_MorphToLoop(&this->skelAnime, &gTwinmoldHeadFlyAnim, 0.0f);
    if (sTwinmoldBattleHandler->unk_1D20 != 0) {
        this->unk_0144 = 10;
    } else {
        this->unk_0144 = 100;
        this->unk_01A8 = 25.0f;
        sTwinmoldBattleHandler->unk_1D7E = 100;
        this->actor.world.pos.x = 0.0f;
        if (sRedTwinmold == this) {
            this->actor.world.pos.z = -1000.0f;
        } else {
            this->actor.world.pos.z = 1400.0f;
        }
    }
    this->actor.world.pos.y = -500.0f;
}

Color_RGBA8 D_809DFA98 = { 185, 140, 70, 255 };

void func_809DAB78(Boss02* this, PlayState* play) {
    s32 pad;
    Player* player = GET_PLAYER(play);
    CollisionPoly* spDC;
    Vec3f spD0;
    f32 spCC;
    f32 spC8;
    f32 spC4;
    s32 i;
    f32 temp_f0;
    f32 phi_f2;
    s16 temp_s0;
    s16 temp_s2;
    Boss02* otherTwinmold = this->otherTwinmold;
    Vec3f spA4;
    f32 spA0;
    f32 sp9C;
    Vec3f sp90;
    CollisionPoly* sp8C;
    s32 bgId;
    Vec3f sp7C;
    Vec3f sp70;
    Vec3f sp64;

    spCC = this->unk_01B0.x - this->actor.world.pos.x;
    spC8 = this->unk_01B0.y - this->actor.world.pos.y;
    spC4 = this->unk_01B0.z - this->actor.world.pos.z;

    if ((this->unk_0144 != 10) && (this->unk_0144 <= 20)) {
        SkelAnime_Update(&this->skelAnime);
        temp_s0 = Math_Atan2S(spCC, spC4);
        temp_s2 = Math_Atan2S(spC8, sqrtf(SQ(spCC) + SQ(spC4)));
        Math_ApproachS(&this->actor.world.rot.y, temp_s0, 0x14, this->unk_0164);
        Math_ApproachS(&this->actor.shape.rot.x, temp_s2, 0x14, this->unk_0164);
        Math_ApproachS(&this->unk_0198, this->unk_019A, 1, 0x20);
        this->unk_0196 += this->unk_0198;
        Math_ApproachF(&this->unk_019C, this->unk_01A0, 0.1f, 100.0f);
        this->unk_01A4 = Math_SinS(this->unk_0196) * this->unk_019C;
        this->actor.world.rot.x = this->actor.shape.rot.x + this->unk_01A4;

        if (!(this->unk_014C & 0x1F) && (sTwinmoldBattleHandler->unk_1D20 == 0)) {
            this->unk_01A0 = Rand_ZeroFloat(0x1000) + 0x800;
            this->unk_019A = Rand_ZeroFloat(0x400) + 0x200;
        }

        if (this->unk_0195 != 0) {
            Math_ApproachF(&this->unk_0164, this->unk_0168 * 1.25f, 1.0f, 62.5f);
        } else {
            Math_ApproachF(&this->unk_0164, this->unk_0168, 1.0f, 50.0f);
        }

        this->unk_0168 = 2000.0f;
        // Boss Overhaul: Boss02_BoSpeedScale slows the dragon down while it breathes
        if (this->unk_0195 != 0) {
            this->actor.speed = this->unk_01A8 * sGiantModeScaleFactor * 1.25f * Boss02_BoSpeedScale(this);
            this->skelAnime.playSpeed = 2.0f;
        } else {
            this->actor.speed = this->unk_01A8 * sGiantModeScaleFactor * Boss02_BoSpeedScale(this);
        }

        Actor_UpdateVelocityWithoutGravity(&this->actor);
        Actor_UpdatePos(&this->actor);

        spD0 = this->actor.world.pos;
        if (sIsInGiantMode) {
            spD0.y = 5000.0f;
        } else {
            spD0.y = 2000.0f;
        }

        temp_f0 = BgCheck_EntityRaycastFloor1(&play->colCtx, &spDC, &spD0);
        if (((this->unk_017C.y < temp_f0) && (temp_f0 <= this->unk_0188.y)) ||
            ((temp_f0 < this->unk_017C.y) && (this->unk_0188.y <= temp_f0))) {
            this->unk_0170 = this->unk_017C;
            this->unk_0170.y = temp_f0;
            this->unk_016C = 120;
            Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_ROAR_OLD);
            Boss02_BoEruption(this, play, &this->unk_0170);
        }

        this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        if (this->unk_0195 != 0) {
            this->actor.world.rot.z = Math_SinS(this->unk_014C * 0x1200) * 0xE00;
        } else {
            this->actor.world.rot.z = Math_SinS(this->unk_014C * 0xC00) * 0xE00;
        }

        this->unk_014E++;
        if (this->unk_014E >= ARRAY_COUNT(this->unk_01BC)) {
            this->unk_014E = 0;
        }

        this->unk_01BC[this->unk_014E].x = this->actor.world.pos.x;
        this->unk_01BC[this->unk_014E].y = this->actor.world.pos.y;
        this->unk_01BC[this->unk_014E].z = this->actor.world.pos.z;

        this->unk_0B1C[this->unk_014E].x = BINANG_TO_RAD_ALT(this->actor.world.rot.x);
        this->unk_0B1C[this->unk_014E].y = BINANG_TO_RAD_ALT(this->actor.world.rot.y);
        this->unk_0B1C[this->unk_014E].z = BINANG_TO_RAD_ALT(this->actor.world.rot.z);
    }

    if ((this->unk_0144 < 10) && (otherTwinmold->unk_0144 >= 20)) {
        this->unk_01B0.y = -1000.0f * sGiantModeScaleFactor;
        if (sIsInGiantMode) {
            this->unk_01B0.y += 3150.0f;
        }
        this->unk_0144 = 3;
        this->unk_0146[0] = 50;
    }

    switch (this->unk_0144) {
        default:
            return;

        case 0:
            if ((TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) && (Rand_ZeroOne() < 0.75f)) {
                this->actor.world.pos.x = player->actor.world.pos.x;
                this->actor.world.pos.z = player->actor.world.pos.z;
                this->actor.world.pos.y = player->actor.world.pos.y - (600.0f * sGiantModeScaleFactor);
            } else {
                this->actor.world.pos.x = Rand_CenteredFloat(5000.0f * sGiantModeScaleFactor);
                this->actor.world.pos.z = Rand_CenteredFloat(5000.0f * sGiantModeScaleFactor);
                this->actor.world.pos.y = -500.0f * sGiantModeScaleFactor;
                if (sIsInGiantMode) {
                    this->actor.world.pos.y += 3150.0f;
                }
            }

            if ((fabsf(this->actor.world.pos.x) < (500.0f * sGiantModeScaleFactor)) &&
                (fabsf(this->actor.world.pos.z) < (500.0f * sGiantModeScaleFactor))) {
                return;
            }

        case 100:
            this->actor.shape.rot.x = 0x4000;
            this->unk_01B0.x = this->actor.world.pos.x;
            this->unk_01B0.y = this->actor.world.pos.y + (1000.0f * sGiantModeScaleFactor);
            this->unk_01B0.z = this->actor.world.pos.z;
            this->unk_0146[0] = 100;
            this->unk_0144 = 1;

        case 1:
            if (this->unk_0146[0] == 0) {
                // Boss Overhaul: hunts Link more often against giant Link and once its twin is dead
                if (Rand_ZeroOne() < Boss02_BoChaseChance(this)) {
                    this->unk_0144 = 5;
                    this->unk_0146[0] = 150;
                } else {
                    this->unk_0144 = 2;
                    this->unk_01B0.x = Rand_CenteredFloat(3000.0f * sGiantModeScaleFactor);
                    this->unk_01B0.z = Rand_CenteredFloat(3000.0f * sGiantModeScaleFactor);
                    if ((fabsf(this->unk_01B0.x) < (500.0f * sGiantModeScaleFactor)) &&
                        (fabsf(this->unk_01B0.z) < (500.0f * sGiantModeScaleFactor))) {
                        this->unk_01B0.x = 500.0f;
                        this->unk_01B0.z = 500.0f;
                    }
                    this->unk_01B0.y =
                        Rand_ZeroFloat(800.0f * sGiantModeScaleFactor) + (200.0f * sGiantModeScaleFactor);
                    if (sIsInGiantMode) {
                        this->unk_01B0.y += 3150.0f;
                    }
                }
                this->unk_0164 = 0.0f;
            }
            return;

        case 2:
            if (this->unk_0195 != 0) {
                phi_f2 = 700.0f;
            } else {
                phi_f2 = 500.0f;
            }
            if (sqrtf(SQ(spCC) + SQ(spC8) + SQ(spC4)) < (phi_f2 * sGiantModeScaleFactor)) {
                this->unk_0144 = 3;
                this->unk_01B0.y = -3000.0f * sGiantModeScaleFactor;
                if (sIsInGiantMode) {
                    this->unk_01B0.y += 3150.0f;
                }
                this->unk_0146[0] = 150;
                this->unk_0164 = 0.0f;
            }
            return;

        case 3:
            if (this->unk_0146[0] == 0) {
                this->unk_0144 = 0;
            }
            return;

        case 5:
            this->unk_01B0.x = player->actor.world.pos.x;
            this->unk_01B0.y = player->actor.world.pos.y + (100.0f * sGiantModeScaleFactor);
            this->unk_01B0.z = player->actor.world.pos.z;
            if (this->unk_0146[0] == 0) {
                this->unk_0144 = 3;
                this->unk_01B0.x = Rand_CenteredFloat(500.0f * sGiantModeScaleFactor) + this->actor.world.pos.x;
                this->unk_01B0.y = -3000.0f * sGiantModeScaleFactor;
                if (sIsInGiantMode) {
                    this->unk_01B0.y += 3150.0f;
                }
                this->unk_01B0.z = Rand_CenteredFloat(500.0f * sGiantModeScaleFactor) + this->actor.world.pos.z;
                this->unk_0146[0] = 150;
                this->unk_0164 = 0.0f;
            }
            return;

        case 10:
            if (this->unk_1678 != 0) {
                this->unk_019A = 0x500;
                this->unk_01A0 = 0x1200;
            } else {
                this->colliderCylinder.dim.radius = 150.0f * sGiantModeScaleFactor;
                this->colliderCylinder.dim.height = 200.0f * sGiantModeScaleFactor;
                this->colliderCylinder.dim.yShift = 0;
                Collider_UpdateCylinder(&this->actor, &this->colliderCylinder);
                CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderCylinder.base);
                CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderCylinder.base);
            }
            return;

        case 11:
            if (this == sRedTwinmold) {
                this->unk_01B0.x = D_809DF9C0[this->unk_1D1A].x;
                this->unk_01B0.y = D_809DF9C0[this->unk_1D1A].y;
                this->unk_01B0.z = D_809DF9C0[this->unk_1D1A].z;
            } else {
                this->unk_01B0.x = D_809DFA2C[this->unk_1D1A].x;
                this->unk_01B0.y = D_809DFA2C[this->unk_1D1A].y;
                this->unk_01B0.z = D_809DFA2C[this->unk_1D1A].z;
            }
            this->actor.shape.rot.x = 0x4000;
            this->unk_0144 = 12;
            this->actor.world.pos.x = this->unk_01B0.x;
            this->actor.world.pos.y = -200.0f;
            this->actor.world.pos.z = this->unk_01B0.z;
            this->unk_01A8 = 25.0f;
            return;

        case 12:
            if (sqrtf(SQ(spCC) + SQ(spC8) + SQ(spC4)) < 500.0f) {
                this->unk_1D1A++;
                this->unk_0164 = 0.0f;
                if (this->unk_1D1A > 8) {
                    this->unk_1D1A = 8;
                }
                if (this == sRedTwinmold) {
                    this->unk_01B0.x = D_809DF9C0[this->unk_1D1A].x;
                    this->unk_01B0.y = D_809DF9C0[this->unk_1D1A].y;
                    this->unk_01B0.z = D_809DF9C0[this->unk_1D1A].z;
                } else {
                    this->unk_01B0.x = D_809DFA2C[this->unk_1D1A].x;
                    this->unk_01B0.y = D_809DFA2C[this->unk_1D1A].y;
                    this->unk_01B0.z = D_809DFA2C[this->unk_1D1A].z;
                }
            }
            return;

        case 20:
            this->unk_0152 = 15;
            if ((s16)(BREG(71) + 140) < this->unk_0146[1]) {
                if (this->unk_0146[0] == 0) {
                    Matrix_RotateYS(Math_Atan2S(-player->actor.world.pos.x, -player->actor.world.pos.z), MTXMODE_NEW);
                    Matrix_MultVecZ(1500.0f * sGiantModeScaleFactor, &spA4);
                    this->unk_0146[0] = 50;
                    this->unk_01B0.x = player->actor.world.pos.x + spA4.x;
                    this->unk_01B0.y =
                        Rand_CenteredFloat(500.0f * sGiantModeScaleFactor) + (600.0f * sGiantModeScaleFactor);
                    if (sIsInGiantMode) {
                        this->unk_01B0.y += 3150.0f;
                    }
                    this->unk_01B0.z = player->actor.world.pos.z + spA4.z;
                }
                this->unk_0168 = 3000.0f;
            } else {
                this->unk_01B0.y += 10.0f * sGiantModeScaleFactor;
                this->unk_0168 = 5000.0f;
            }
            this->unk_019A = 0x1000;
            this->unk_01A8 = 25.0f;
            this->unk_01A0 = 0x1800;
            if (this->unk_0146[1] == 0) {
                this->unk_015C = 1;
                this->unk_0144 = 21;
                this->unk_0146[0] = 20;
                this->unk_0152 = 0;
                sTwinmoldBattleHandler->unk_1D20 = 102;
                sTwinmoldBattleHandler->subCamAtVel = 0.0f;
                Audio_PlaySfx(NA_SE_EN_INBOSS_DEAD_PRE2_OLD);
            } else if (!(this->unk_0146[1] & 0xF) && (Rand_ZeroOne() < 0.5f)) {
                Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_DAMAGE_OLD);
            }
            return;

        case 21:
            this->unk_01A8 = 0.0f;
            this->actor.speed = 0.0f;
            if (this->unk_0146[0] == 0) {
                this->unk_0146[0] = 3;

                for (i = 0; i < 35; i++) {
                    Boss02_SpawnEffectFragment(play->specialEffects, &this->unk_147C[this->unk_1678]);
                }

                Boss02_SpawnEffectFlash(play->specialEffects, &this->unk_147C[this->unk_1678]);
                Audio_PlaySfx(NA_SE_EV_EXPLOSION);

                this->unk_1678--;
                if (this->unk_1678 <= 0) {
                    this->unk_0144 = 22;
                    this->actor.gravity = -1.0f * sGiantModeScaleFactor;
                    this->actor.velocity.y = 0.0f;
                    this->actor.terminalVelocity = -1000.0f * sGiantModeScaleFactor;
                    this->unk_0164 = Rand_CenteredFloat(0.05f);

                    spCC = player->actor.world.pos.x - this->actor.world.pos.x;
                    spC4 = player->actor.world.pos.z - this->actor.world.pos.z;
                    if (sqrtf(SQ(spCC) + SQ(spC4)) < (400.0f * sGiantModeScaleFactor)) {
                        this->actor.speed = 15.0f * sGiantModeScaleFactor;
                    }

                    spCC = this->actor.world.pos.x;
                    spC4 = this->actor.world.pos.z;
                    if (sqrtf(SQ(spCC) + SQ(spC4)) < (400.0f * sGiantModeScaleFactor)) {
                        this->actor.speed = 15.0f * sGiantModeScaleFactor;
                    }

                    if (otherTwinmold->unk_0144 >= 10) {
                        SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, NA_BGM_CLEAR_BOSS | SEQ_FLAG_ASYNC);
                    }

                    Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_DEAD_OLD);
                }
            }
            return;

        case 22:
            i = (this->unk_014E + 196) % ARRAY_COUNT(this->unk_01BC);

            Math_Vec3f_Copy(&this->unk_01BC[i], &this->actor.world.pos);
            this->unk_0B1C[i].y += this->unk_0164;
            Math_ApproachF(&this->unk_0B1C[i].x, -(M_PIf / 2), 0.1f, 0.07f);
            Actor_MoveWithGravity(&this->actor);
            Actor_UpdateBgCheckInfo(play, &this->actor, 50.0f, 150.0f, 100.0f, UPDBGCHECKINFO_FLAG_4);

            if (this->actor.bgCheckFlags & BGCHECKFLAG_GROUND) {
                this->unk_0144 = 23;
                this->actor.speed = 0.0f;
                this->unk_0170 = this->unk_017C;
                this->unk_016C = 30;
                this->unk_0170.y = this->actor.floorHeight;
                sTwinmoldBattleHandler->unk_1D20 = 103;
                sTwinmoldBattleHandler->unk_1D1C = 0;
                sTwinmoldBattleHandler->unk_0146[0] = 15;
                sTwinmoldBattleHandler->unk_0150 = 0;
                Audio_PlaySfx(NA_SE_EV_LIGHTNING);

                for (i = 0; i < 30; i++) {
                    Boss02_SpawnEffectFragment(play->specialEffects, &this->unk_0170);
                }

                this->unk_0146[0] = 10;
            }
            break;

        case 23:
            i = (this->unk_014E + 196) % ARRAY_COUNT(this->unk_01BC);
            Math_Vec3f_Copy(&this->unk_01BC[i], &this->actor.world.pos);
            Math_ApproachF(&this->unk_0B1C[i].x, -(M_PIf / 2), 0.05f, 0.07f);

            if (this->unk_0146[0] & 1) {
                sp9C = Rand_ZeroFloat(M_PIf);

                for (i = 0; i < 15; i++) {
                    Matrix_RotateYF(((2.0f * (i * M_PIf)) / 15.0f) + sp9C, MTXMODE_NEW);
                    Matrix_MultVecZ((10 - this->unk_0146[0]) * (sGiantModeScaleFactor * 300.0f) * 0.1f, &sp90);
                    spD0.x = this->unk_0170.x + sp90.x;
                    spD0.y = this->unk_0170.y + (1000.0f * sGiantModeScaleFactor);
                    spD0.z = this->unk_0170.z + sp90.z;
                    if (BgCheck_EntityRaycastFloor3(&play->colCtx, &sp8C, &bgId, &spD0) != BGCHECK_Y_MIN) {
                        spA0 = BgCheck_EntityRaycastFloor1(&play->colCtx, &sp8C, &spD0);
                        Matrix_MultVecZ(5.0f * sGiantModeScaleFactor, &sp70);
                        sp70.y = 2.0f * sGiantModeScaleFactor;
                        sp64.y = 0.3f * sGiantModeScaleFactor;
                        sp64.z = 0.0f;
                        sp64.x = 0.0f;
                        sp7C.x = spD0.x;
                        sp7C.y = spA0;
                        sp7C.z = spD0.z;
                        func_800B0F18(play, &sp7C, &sp70, &sp64, &D_809DFA98, &D_809DFA98,
                                      (Rand_ZeroFloat(500.0f) + 1200.0f) * sGiantModeScaleFactor,
                                      20.0f * sGiantModeScaleFactor, Rand_ZeroFloat(5.0f) + 14.0f);
                    }
                }
            }
    }

    this->colliderCylinder.dim.radius = 150.0f * sGiantModeScaleFactor;
    this->colliderCylinder.dim.height = 200.0f * sGiantModeScaleFactor;
    this->colliderCylinder.dim.yShift = 0;
    Collider_UpdateCylinder(&this->actor, &this->colliderCylinder);
    CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderCylinder.base);
}

void func_809DBFB4(Boss02* this, PlayState* play) {
    Boss02* otherTwinmold = this->otherTwinmold;
    s32 temp;
    s32 i;
    u8 damage;

    if (this->unk_0154 == 0) {
        if (sIsInGiantMode) {
            temp = 0;
        } else {
            temp = 1;
        }

        for (i = temp; i < ARRAY_COUNT(this->colliderSphere2Elements); i++) {
            if (this->colliderSphere2.elements[i].base.acElemFlags & ACELEM_HIT) {
                this->colliderSphere2.elements[i].base.acElemFlags &= ~ACELEM_HIT;
                this->unk_0154 = 15;
                this->unk_0156 = 15;

                if (i == 0) {
                    Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_DAMAGE_OLD);
                    this->unk_015C = 1;
                } else {
                    Audio_PlaySfx_AtPos(&this->unk_167C, NA_SE_EN_INBOSS_DAMAGE_OLD);
                    this->unk_015C = 10;
                }

                if (sIsInGiantMode) {
                    this->unk_0152 = 15;
                } else {
                    this->unk_0152 = 12;
                }

                damage = this->actor.colChkInfo.damage;

                if (otherTwinmold->unk_0144 < 20) {
                    this->actor.colChkInfo.health -= damage;

                    if ((s8)this->actor.colChkInfo.health <= 0) {
                        Actor_Kill(this->actor.child);
                        this->actor.child = NULL;
                        Enemy_StartFinishingBlow(play, &this->actor);
                        this->skelAnime.playSpeed = 2.0f;
                        this->unk_0144 = 20;

                        if (otherTwinmold->unk_0144 >= 10) {
                            SEQCMD_STOP_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 1);
                            GameInteractor_ExecuteOnBossDefeated(this->actor.id); // 2S2H Time Splits
                        } else {
                            otherTwinmold->unk_0195 = 1;
                        }

                        this->unk_0146[1] = BREG(70) + 170;
                        this->unk_0146[0] = 0;
                        this->actor.shape.rot.x = 0x4000;

                        if (!sIsInGiantMode) {
                            if (this->actor.world.pos.y < -200.0f) {
                                this->actor.world.pos.y = -200.0f;
                            }
                        } else if (this->actor.world.pos.y < 3130.0f) {
                            this->actor.world.pos.y = 3130.0f;
                        }

                        sTwinmoldBattleHandler->unk_1D20 = 100;

                        if (this == sRedTwinmold) {
                            sTwinmoldBattleHandler->unk_0194 = 0;
                        } else {
                            sTwinmoldBattleHandler->unk_0194 = 1;
                        }
                    }
                }
            }
        }
    }
}

void Boss02_Tail_Update(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;
    s32 pad;
    Vec3f pos;
    CollisionPoly* outPoly;

    this->unk_014C++;
    if ((this->unk_014C + this->actor.params) & 1) {
        pos = this->actor.world.pos;
        this->actor.focus.pos = this->actor.world.pos;
        if (sIsInGiantMode) {
            pos.y = 5000.0f;
        } else {
            pos.y = 2000.0f;
        }

        if ((this->actor.focus.pos.y < BgCheck_EntityRaycastFloor1(&play->colCtx, &outPoly, &pos)) || sIsInGiantMode) {
            this->actor.flags &= ~ACTOR_FLAG_ATTENTION_ENABLED;
        } else {
            this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
        }
    }
}

void Boss02_Twinmold_Update(Actor* thisx, PlayState* play) {
    Vec3f sp3C;
    Boss02* this = (Boss02*)thisx;
    s32 pad;
    s16 i;

    if (!sIsInGiantMode) {
        Actor_SetScale(&this->actor, 0.6f);
    } else {
        Actor_SetScale(&this->actor, 0.060000001f);
    }

    if (sTwinmoldBattleHandler->giantsMaskCsState == GIANTS_MASK_CS_STATE_WAITING_FOR_MASK) {
        for (i = 0; i < ARRAY_COUNT(this->unk_0146); i++) {
            if (this->unk_0146[i] != 0) {
                this->unk_0146[i]--;
            }
        }

        if (this->unk_016C != 0) {
            this->unk_016C--;
        }

        if (this->unk_0154 != 0) {
            this->unk_0154--;
        }

        if (this->unk_0156 != 0) {
            this->unk_0156--;
        }

        if (this->unk_0152 != 0) {
            this->unk_0152--;
        }

        this->unk_014C++;

        this->actionFunc(this, play);

        if ((TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ||
            (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_BLUE)) {
            Boss02_BoDragonUpdate(this, play);
        }

        if (this->unk_016C != 0) {
            if ((this->unk_016C == 60) && (this->unk_0144 < 20)) {
                Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_ROAR_OLD);
            }
            Actor_PlaySfx(&this->actor, NA_SE_EN_INBOSS_SAND_OLD - SFX_FLAG);

            if (this->unk_0144 > 20) {
                sp3C.x = Rand_CenteredFloat(100.0f * sGiantModeScaleFactor) + this->unk_0170.x;
                sp3C.y = Rand_CenteredFloat(50.0f * sGiantModeScaleFactor) + this->unk_0170.y;
                sp3C.z = Rand_CenteredFloat(100.0f * sGiantModeScaleFactor) + this->unk_0170.z;
                Boss02_SpawnEffectSand(play->specialEffects, &sp3C, Rand_ZeroFloat(3.0f) + 6.0f);
            }

            if ((this->unk_014C % 2) == 0) {
                sp3C.x = Rand_CenteredFloat(100.0f * sGiantModeScaleFactor) + this->unk_0170.x;
                sp3C.y = Rand_CenteredFloat(50.0f * sGiantModeScaleFactor) + this->unk_0170.y;
                sp3C.z = Rand_CenteredFloat(100.0f * sGiantModeScaleFactor) + this->unk_0170.z;
                Boss02_SpawnEffectSand(play->specialEffects, &sp3C, Rand_ZeroFloat(3.0f) + 6.0f);
            }
        }

        if ((this->unk_0144 < 20) && (this->unk_0144 != 10)) {
            func_809DBFB4(this, play);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderSphere1.base);
            CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderSphere1.base);
            CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderSphere1.base);
            CollisionCheck_SetAC(play, &play->colChkCtx, &this->colliderSphere2.base);
            CollisionCheck_SetAT(play, &play->colChkCtx, &this->colliderSphere2.base);
            CollisionCheck_SetOC(play, &play->colChkCtx, &this->colliderSphere2.base);
        }

        switch (this->unk_015C) {
            case 1:
                this->unk_015A = 0;
                this->unk_0158 = this->unk_015A;
                this->unk_015C++;

            case 2:
                this->unk_015A += 2;
                if (this->unk_015A > 20) {
                    this->unk_015C++;
                }
                this->unk_0156 |= 4;
                break;

            case 3:
                this->unk_0158 += 2;
                if (this->unk_0158 > 20) {
                    this->unk_015C = 0;
                }
                this->unk_0156 |= 4;
                break;

            case 10:
                this->unk_015A = 21;
                this->unk_0158 = this->unk_015A;
                this->unk_015C++;

            case 11:
                this->unk_0158 -= 2;
                if (this->unk_0158 <= 0) {
                    this->unk_0158 = 0;
                    this->unk_015C++;
                }
                this->unk_0156 |= 4;
                break;

            case 12:
                this->unk_015A -= 2;
                if (this->unk_015A <= 0) {
                    this->unk_015A = 0;
                    this->unk_015C = 0;
                }
                this->unk_0156 |= 4;
                break;
        }
    } else if ((TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) ||
               (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_BLUE)) {
        // Boss Overhaul: during the Giant's Mask transformation the jaws go dark and any breath is dropped
        this->boWindupTimer = 0;
        this->boBreathTimer = 0;
        this->boGlow = 0.0f;
        this->boSpeedScale = 1.0f;
        Boss02_BoUpdateLight(this);
    }
}

void Boss02_BattleHandler_Update(Actor* thisx, PlayState* play) {
    Boss02* this = (Boss02*)thisx;

    this->giantModeScaleFactor = sGiantModeScaleFactor;
    play->envCtx.sandstormState = SANDSTORM_D;

    if (sBlueWarp != NULL) {
        this->fogNear = KREG(23) + -15.0f;
        D_801F4E30 = 0;
        play->envCtx.lightSettingOverride = 1;
        play->skyboxId = SKYBOX_NORMAL_SKY;
    } else if (!sIsInGiantMode) {
        this->fogNear = 0.0f;
        D_801F4E30 = this->unk_1D7C;
        play->envCtx.lightSettingOverride = 0;
        play->skyboxId = SKYBOX_2;
    } else {
        this->fogNear = KREG(23) + -15.0f;
        D_801F4E30 = ((KREG(24) * 0.1f) + 1.0f) * this->unk_1D7C;
        play->envCtx.lightSettingOverride = 1;
        play->skyboxId = SKYBOX_NORMAL_SKY;
    }

    Math_ApproachS(&this->unk_1D7C, this->unk_1D7E, 1, 3);

    if (this->unk_0146[0] != 0) {
        this->unk_0146[0]--;
    }

    if (sBlueWarp == NULL) {
        if (sTwinmoldMusicStartTimer != 0) {
            sTwinmoldMusicStartTimer--;
            if (sTwinmoldMusicStartTimer == 0) {
                SEQCMD_PLAY_SEQUENCE(SEQ_PLAYER_BGM_MAIN, 0, NA_BGM_BOSS | SEQ_FLAG_ASYNC);
            }
        }
    }

    Boss02_UpdateEffects(play);
    func_809DEAC4(this, play);
    Boss02_HandleGiantsMaskCutscene(this, play);
}

Gfx* D_809DFA9C[] = {
    gTwinmoldBodySegment1DL,  gTwinmoldBodySegment2DL,  gTwinmoldBodySegment3DL,  gTwinmoldBodySegment4DL,
    gTwinmoldBodySegment5DL,  gTwinmoldBodySegment6DL,  gTwinmoldBodySegment7DL,  gTwinmoldBodySegment8DL,
    gTwinmoldBodySegment9DL,  gTwinmoldBodySegment10DL, gTwinmoldBodySegment11DL, gTwinmoldBodySegment12DL,
    gTwinmoldBodySegment13DL, gTwinmoldBodySegment14DL, gTwinmoldBodySegment15DL, gTwinmoldBodySegment16DL,
    gTwinmoldBodySegment17DL, gTwinmoldBodySegment18DL, gTwinmoldBodySegment19DL, gTwinmoldBodySegment20DL,
    gTwinmoldBodySegment21DL, gTwinmoldBodyTailDL,
};

Vec3f D_809DFAF4 = { -10000.0f, -100000.0f, -100000.0f };

void Boss02_Twinmold_Draw(Actor* thisx, PlayState* play2) {
    PlayState* play = play2;
    Boss02* this = (Boss02*)thisx;
    s32 i;
    s32 idx;
    Mtx* mtxHead = GRAPH_ALLOC(play->state.gfxCtx, 23 * sizeof(Mtx));
    Mtx* mtx = mtxHead;
    s32 phi_v0;
    f32 phi_f12;
    f32 spAC;
    f32 spA8;
    f32 spA4;
    f32 spA0;
    f32 sp9C;
    f32 sp98;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    if (TWINMOLD_GET_TYPE(&this->actor) == TWINMOLD_TYPE_RED) {
        gSPSegment(POLY_OPA_DISP++, 0x08, Lib_SegmentedToVirtual(gTwinmoldRedSkinTex));
    } else {
        gSPSegment(POLY_OPA_DISP++, 0x08, Lib_SegmentedToVirtual(gTwinmoldBlueSkinTex));
    }

    gSPSegment(POLY_OPA_DISP++, 0x0D, mtxHead);

    if (!sIsInGiantMode) {
        sp98 = -500.0f;
    } else {
        sp98 = 3100.0f;
    }

    sp9C = this->unk_0152 * (M_PIf / 4) * (1.0f / 15);
    if (this->unk_0144 < 20) {
        spAC = Math_SinS(this->unk_014C * 0x2200) * sp9C * 0.5f;
        spA8 = Math_CosS(this->unk_014C * 0x3200) * sp9C;
    } else {
        spAC = Math_SinS(this->unk_014C * 0x1000) * sp9C * 0.5f;
        spA8 = Math_CosS(this->unk_014C * 0x1500) * sp9C;
    }

    idx = (this->unk_014E + 196) % ARRAY_COUNT(this->unk_01BC);
    Matrix_Translate(this->unk_01BC[idx].x, this->unk_01BC[idx].y, this->unk_01BC[idx].z, MTXMODE_NEW);
    Matrix_RotateYF(this->unk_0B1C[idx].y + spA8, MTXMODE_APPLY);
    Matrix_RotateXFApply(spAC - this->unk_0B1C[idx].x);
    Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
    Matrix_Translate(0.0f, 0.0f, 250.0f, MTXMODE_APPLY);
    Matrix_MultVecZ(150.0f, &this->unk_147C[0]);
    this->unk_0188 = this->unk_017C;
    Matrix_MultVecZ(300.0f, &this->unk_017C);
    Math_Vec3f_Copy(&this->actor.focus.pos, &this->unk_017C);

    if ((this->unk_0156 & 1) && (this->unk_0158 == 0)) {
        POLY_OPA_DISP = Gfx_SetFog(POLY_OPA_DISP, 255, 0, 0, 255, 0x384, 0x44B);
    }

    SkelAnime_DrawOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, NULL, NULL, &this->actor);
    POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);

    spA4 = 0.0f;
    spA0 = 0.0f;
    for (i = 0; i < ARRAY_COUNT(D_809DFA9C); i++, mtx++) {
        if (this->unk_0195 != 0) {
            phi_v0 = (D_809DF5E4[i + 1] + this->unk_014E) % ARRAY_COUNT(this->unk_01BC);
        } else {
            phi_v0 = (D_809DF5B4[i + 1] + this->unk_014E) % ARRAY_COUNT(this->unk_01BC);
        }

        if ((i == 21) && (this->unk_0144 < 20)) {
            spA4 = spAC;
            spA0 = spA8;
        }

        Matrix_Translate(this->unk_01BC[phi_v0].x, this->unk_01BC[phi_v0].y, this->unk_01BC[phi_v0].z, MTXMODE_NEW);
        Matrix_RotateYF(this->unk_0B1C[phi_v0].y + spA0, MTXMODE_APPLY);
        Matrix_RotateXFApply(spA4 - this->unk_0B1C[phi_v0].x);
        Matrix_RotateZF(this->unk_0B1C[phi_v0].z, MTXMODE_APPLY);
        Matrix_Scale(this->actor.scale.x, this->actor.scale.y, this->actor.scale.z, MTXMODE_APPLY);
        Matrix_RotateYF(M_PIf / 2, MTXMODE_APPLY);
        Matrix_RotateXFApply(-(M_PIf / 2));
        Matrix_ToMtx(mtx);

        gSPMatrix(POLY_OPA_DISP++, mtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);

        if ((this->unk_0156 & 1) && (i >= this->unk_0158) && (this->unk_015A >= i)) {
            POLY_OPA_DISP = Gfx_SetFog(POLY_OPA_DISP, 255, 0, 0, 255, 0x384, 0x44B);
        }

        if (i < this->unk_1678) {
            if (sp98 < this->unk_01BC[phi_v0].y) {
                gSPDisplayList(POLY_OPA_DISP++, D_809DFA9C[i]);
            }
            POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);
        }

        if (i == 21) {
            phi_f12 = 600.0f;
        } else {
            phi_f12 = 200.0f;
        }

        Matrix_MultVecX(phi_f12, &this->unk_147C[i + 1]);

        if (i == 21) {
            Actor* child = this->actor.child;

            if (child != NULL) {
                Matrix_MultVecX(500.0f, &this->actor.child->world.pos);
            }

            func_809DA50C(1, &this->colliderSphere2, &this->unk_147C[i + 1]);
            SkinMatrix_Vec3fMtxFMultXYZW(&play->viewProjectionMtxF, &this->unk_147C[i + 1], &this->unk_167C,
                                         &this->actor.projectedW);
        } else {
            func_809DA50C(i, &this->colliderSphere1, &this->unk_147C[i + 1]);
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);

    if (!sIsInGiantMode) {
        func_809DA50C(0, &this->colliderSphere2, &D_809DFAF4);
        func_809DA50C(21, &this->colliderSphere1, &this->unk_147C[0]);
    } else {
        func_809DA50C(0, &this->colliderSphere2, &this->unk_147C[0]);
        func_809DA50C(21, &this->colliderSphere1, &D_809DFAF4);
    }

    // Boss Overhaul: the glowing jaws
    Boss02_BoDrawMouth(this, play);
}

void Boss02_BattleHandler_Draw(Actor* thisx, PlayState* play) {
    Boss02_DrawEffects(play);
}

void Boss02_UpdateEffects(PlayState* play) {
    TwinmoldEffect* effect = (TwinmoldEffect*)play->specialEffects;
    f32 floorY;
    s16 i;

    if (!sIsInGiantMode) {
        floorY = 0.0f;
    } else {
        floorY = 3150.0f;
    }

    for (i = 0; i < TWINMOLD_EFFECT_COUNT; i++, effect++) {
        if (effect->type) {
            effect->timer++;
            effect->pos.x += effect->velocity.x * sGiantModeScaleFactor;
            effect->pos.y += effect->velocity.y * sGiantModeScaleFactor;
            effect->pos.z += effect->velocity.z * sGiantModeScaleFactor;
            effect->velocity.y += effect->accel.y;

            if (effect->type < TWINMOLD_EFFECT_FRAGMENT) {
                Math_ApproachF(&effect->scale, effect->targetScale, 0.1f, 0.1f);
                if (effect->type == TWINMOLD_EFFECT_BLACK_DUST) {
                    effect->alpha -= 18;
                } else {
                    effect->alpha -= 15;
                }

                if (effect->alpha <= 0) {
                    effect->type = TWINMOLD_EFFECT_NONE;
                }
            } else if (effect->type == TWINMOLD_EFFECT_FRAGMENT) {
                effect->rotX += 0x1000;
                effect->rotY += 0x1500;
                if ((effect->pos.y < floorY) || (effect->timer > 50)) {
                    effect->type = TWINMOLD_EFFECT_NONE;
                }
            } else if (effect->type == TWINMOLD_EFFECT_FLASH) {
                Math_ApproachF(&effect->scale, 80.0f, 0.2f, 20.0f);
                effect->alpha -= 15;
                if (effect->alpha <= 0) {
                    effect->type = TWINMOLD_EFFECT_NONE;
                }
            }
        }
    }
}

void Boss02_DrawEffects(PlayState* play) {
    u8 flag = false;
    TwinmoldEffect* effect = (TwinmoldEffect*)play->specialEffects;
    s16 i;
    u8 alpha;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    for (i = 0; i < TWINMOLD_EFFECT_COUNT; i++, effect++) {
        if (effect->type == TWINMOLD_EFFECT_SAND) {
            // Here and below, key by effect type merged with epoch
            FrameInterpolation_RecordOpenChild(effect, (effect->epoch << 4) | effect->type);
            FrameInterpolation_IgnoreActorMtx();
            if (!flag) {
                gSPDisplayList(POLY_XLU_DISP++, gTwinmoldDustMaterialDL);
                gDPSetEnvColor(POLY_XLU_DISP++, 185, 140, 70, 128);
                flag++;
            }

            alpha = effect->alpha;
            if (effect->alpha > 255) {
                alpha = 255;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 185, 140, 70, alpha);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, effect->timer + (i * 3), (effect->timer + (i * 3)) * 5,
                                          32, 64, 1, 0, 0, 32, 32, 1, 5, 0, 0));

            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(effect->scale * sGiantModeScaleFactor, effect->scale * sGiantModeScaleFactor, 1.0f,
                         MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gTwinmoldDustModelDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    effect = (TwinmoldEffect*)play->specialEffects;
    for (i = 0, flag = false; i < TWINMOLD_EFFECT_COUNT; i++, effect++) {
        if (effect->type == TWINMOLD_EFFECT_FRAGMENT) {
            FrameInterpolation_RecordOpenChild(effect, (effect->epoch << 4) | effect->type);
            FrameInterpolation_IgnoreActorMtx();
            if (!flag) {
                gDPSetCombineLERP(POLY_OPA_DISP++, SHADE, 0, PRIMITIVE, 0, SHADE, 0, PRIMITIVE, 0, SHADE, 0, PRIMITIVE,
                                  0, SHADE, 0, PRIMITIVE, 0);
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x01, 100, 100, 120, 255);
                flag++;
            }

            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_RotateYS(effect->rotY, MTXMODE_APPLY);
            Matrix_RotateXS(effect->rotX, MTXMODE_APPLY);
            Matrix_Scale(effect->scale * sGiantModeScaleFactor, effect->scale * sGiantModeScaleFactor,
                         effect->scale * sGiantModeScaleFactor, MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_OPA_DISP++, gEffFragments1DL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    effect = (TwinmoldEffect*)play->specialEffects;
    for (i = 0, flag = false; i < TWINMOLD_EFFECT_COUNT; i++, effect++) {
        if (effect->type == TWINMOLD_EFFECT_FLASH) {
            FrameInterpolation_RecordOpenChild(effect, (effect->epoch << 4) | effect->type);
            FrameInterpolation_IgnoreActorMtx();
            if (!flag) { //! @bug - dev forgot to set flag to 1, should only apply to first entry?
                gSPDisplayList(POLY_XLU_DISP++, gLightOrbMaterial1DL);
                gDPSetEnvColor(POLY_XLU_DISP++, 255, 0, 0, 128);
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 200, (u8)effect->alpha);

            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(effect->scale * sGiantModeScaleFactor, effect->scale * sGiantModeScaleFactor, 1.0f,
                         MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gLightOrbModelDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    effect = (TwinmoldEffect*)play->specialEffects;
    for (i = 0, flag = false; i < TWINMOLD_EFFECT_COUNT; i++, effect++) {
        if (effect->type == TWINMOLD_EFFECT_BLACK_DUST) {
            FrameInterpolation_RecordOpenChild(effect, (effect->epoch << 4) | effect->type);
            FrameInterpolation_IgnoreActorMtx();
            if (!flag) {
                gSPDisplayList(POLY_XLU_DISP++, gTwinmoldDustMaterialDL);
                gDPSetEnvColor(POLY_XLU_DISP++, 30, 30, 30, 128);
                flag++;
            }

            gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 30, 30, 30, (u8)effect->alpha);
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, effect->timer + (i * 3), (effect->timer + (i * 3)) * 5,
                                          32, 64, 1, 0, 0, 32, 32, 1, 5, 0, 0));

            Matrix_Translate(effect->pos.x, effect->pos.y, effect->pos.z, MTXMODE_NEW);
            Matrix_ReplaceRotation(&play->billboardMtxF);
            Matrix_Scale(effect->scale * sGiantModeScaleFactor, effect->scale * sGiantModeScaleFactor, 1.0f,
                         MTXMODE_APPLY);

            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
            gSPDisplayList(POLY_XLU_DISP++, gTwinmoldDustModelDL);
            FrameInterpolation_RecordCloseChild();
        }
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * This function is primarily responsible for handling the cutscene where the player puts on
 * or takes off the Giant's Mask. It does some manual camera control, adjusts the player's
 * scale, switches between the "Normal" and "Giant" variations of the arena, and creates a
 * "flash" when the cutscene ends by filling the screen with white. In addition, this function
 * is responsible for updating the EnvironmentContext's fogNear variable, which it does even
 * when the Giant's Mask cutscene is not playing.
 */
void Boss02_HandleGiantsMaskCutscene(Boss02* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s16 i;
    Actor* actorIter;
    Vec3f subCamEyeOffset;
    u8 switchArena = false;
    f32 scale;
    s16 alpha;

    this->giantsMaskCsTimer++;

    switch (this->giantsMaskCsState) {
        case GIANTS_MASK_CS_STATE_WAITING_FOR_MASK:
            if (player->stateFlags1 & PLAYER_STATE1_100) {
                Cutscene_StartManual(play, &play->csCtx);
                this->subCamId = Play_CreateSubCamera(play);
                Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
                Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
                Play_EnableMotionBlur(150);
                this->giantsMaskCsTimer = 0;
                this->subCamAtVel = 0.0f;
                this->subCamUpRotZScale = 0.0f;
                if (!sIsInGiantMode) {
                    this->giantsMaskCsState = GIANTS_MASK_CS_STATE_MASK_ON;
                    this->subCamEyeOffsetY = 10.0f;
                    this->subCamDistZFromPlayer = 60.0f;
                    this->subCamAtOffsetY = 23.0f;
                    this->playerScale = 0.01f;
                    this->fogNear = 0.0f;
                    goto maskOn; // Jumps to this label to prevent the player from skipping the cutscene immediately.
                } else {
                    this->giantsMaskCsState = GIANTS_MASK_CS_STATE_MASK_OFF;
                    this->subCamEyeOffsetY = 10.0f;
                    this->subCamDistZFromPlayer = 200.0f;
                    this->subCamAtOffsetY = 273.0f;
                    this->playerScale = 0.1f;
                    this->fogNear = -100.0f;
                    switchArena = true;
                    goto maskOff; // Jumps to this label to prevent the player from skipping the cutscene immediately.
                }
            }
            break;

        case GIANTS_MASK_CS_STATE_MASK_ON:
            if ((this->giantsMaskCsTimer < 80) && sCanSkipMaskOnCs &&
                CHECK_BTN_ANY(CONTROLLER1(&play->state)->press.button,
                              BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT | BTN_DPAD_EQUIP)) {
                this->giantsMaskCsState++;
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_STARTED;
                this->giantsMaskCsTimer = 0;
                break;
            }

        maskOn:
            if (this->giantsMaskCsTimer >= 50) {
                if (this->giantsMaskCsTimer == (u32)(BREG(43) + 60)) {
                    Audio_PlaySfx(NA_SE_PL_TRANSFORM_GIANT);
                }

                // Makes the camera quickly move away from the player and look upwards as the player
                // increases in size. Also decreases the fogNear from 0.0f to -100.0f.
                Math_ApproachF(&this->subCamDistZFromPlayer, 200.0f, 0.1f, this->subCamAtVel * 640.0f);
                Math_ApproachF(&this->subCamAtOffsetY, 273.0f, 0.1f, this->subCamAtVel * 150.0f);
                Math_ApproachF(&this->playerScale, 0.1f, 0.2f, this->subCamAtVel * 0.1f);
                Math_ApproachF(&this->fogNear, -100.0f, 1.0f, this->subCamAtVel * 100.0f);
                Math_ApproachF(&this->subCamAtVel, 1.0f, 1.0f, 0.001f);
            } else {
                // Makes the camera slowly approach the player.
                Math_ApproachF(&this->subCamDistZFromPlayer, 30.0f, 0.1f, 1.0f);
            }

            // Makes the camera roll side-to-side at the start of the cutscene, and stops
            // the camera from rolling after 51 frames have passed.
            if (this->giantsMaskCsTimer > 50) {
                Math_ApproachZeroF(&this->subCamUpRotZScale, 1.0f, 0.06f);
            } else {
                Math_ApproachF(&this->subCamUpRotZScale, 0.4f, 1.0f, 0.02f);
            }

            if (this->giantsMaskCsTimer == 107) {
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_STARTED;
            }

            if (this->giantsMaskCsTimer > 120) {
                switchArena = true;
                sCanSkipMaskOnCs = true;
                goto done;
            }
            break;

        case GIANTS_MASK_CS_STATE_MASK_ON_SKIPPED:
            if (this->giantsMaskCsTimer >= 8) {
                switchArena = true;
                goto done;
            }
            break;

        case GIANTS_MASK_CS_STATE_MASK_OFF:
            if ((this->giantsMaskCsTimer < 30) && sCanSkipMaskOffCs &&
                CHECK_BTN_ANY(CONTROLLER1(&play->state)->press.button,
                              BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT | BTN_DPAD_EQUIP)) {
                this->giantsMaskCsState++;
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_STARTED;
                this->giantsMaskCsTimer = 0;
                break;
            }

        maskOff:
            if (this->giantsMaskCsTimer != 0) {
                if (this->giantsMaskCsTimer == (u32)(BREG(44) + 10)) {
                    Audio_PlaySfx(NA_SE_PL_TRANSFORM_NORAML);
                }

                // Makes the camera move slowly towards the player and look downwards as the player
                // decreases in size. Also increases the fogNear from -100.0f to 0.0f.
                Math_ApproachF(&this->subCamDistZFromPlayer, 60.0f, 0.1f, this->subCamAtVel * 640.0f);
                Math_ApproachF(&this->subCamAtOffsetY, 23.0f, 0.1f, this->subCamAtVel * 150.0f);
                Math_ApproachF(&this->playerScale, 0.01f, 0.1f, 0.003f);
                Math_ApproachF(&this->fogNear, 0.0f, 1.0f, this->subCamAtVel * 100.0f);
                Math_ApproachF(&this->subCamAtVel, 2.0f, 1.0f, 0.01f);
            }

            if (this->giantsMaskCsTimer == 42) {
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_STARTED;
            }

            if (this->giantsMaskCsTimer > 50) {
                sCanSkipMaskOffCs = true;
                goto done;
            }
            break;

        case GIANTS_MASK_CS_STATE_MASK_OFF_SKIPPED:
            if (this->giantsMaskCsTimer >= 8) {
                goto done;
            }
            break;

        done:
        case GIANTS_MASK_CS_STATE_DONE:
            // Ends the cutscene and restores the player's scale to its "original" value of 0.1f.
            // This might seem strange in the case where the player has put on the Giant's Mask,
            // since they seem so large after the cutscene ends. However, the player is still
            // their normal size; the arena and everything in it has shrunk instead.
            this->giantsMaskCsState = GIANTS_MASK_CS_STATE_WAITING_FOR_MASK;
            func_80169AFC(play, this->subCamId, 0);
            this->subCamId = SUB_CAM_ID_DONE;
            Cutscene_StopManual(play, &play->csCtx);
            this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
            player->stateFlags1 &= ~PLAYER_STATE1_100;
            this->playerScale = 0.01f;
            Play_DisableMotionBlur();
            break;
    }

    if (player->transformation == PLAYER_FORM_FIERCE_DEITY) {
        Actor_SetScale(&player->actor, this->playerScale * 1.5f);
    } else {
        Actor_SetScale(&player->actor, this->playerScale);
    }

    play->envCtx.adjLightSettings.fogNear = this->fogNear;

    // Switches from the "normal" variation of the arena to the "giant" variation (or vice versa)
    // at the appropriate point in the Giant's Mask cutscene.
    if (switchArena) {
        sIsInGiantMode = true - sIsInGiantMode;
        if (!sIsInGiantMode) {
            sGiantModeScaleFactor = 1.0f;
        } else {
            sGiantModeScaleFactor = 0.1f;
        }

        this->giantModeScaleFactor = sGiantModeScaleFactor;

        if (!sIsInGiantMode) {
            // In this case, the player is switching from the "giant" arena to the "normal" arena.
            // The "giant" arena sits 3150 units above the "normal" arena, so this moves the player,
            // Twinmold, and the blue warp down and scales their world coordinates upwards to be
            // appropriate for the larger arena.
            if (sBlueWarp != NULL) {
                sBlueWarp->unk_203 = 0;
                sBlueWarp->unk_204 = 1.0f;
                sBlueWarp->dyna.actor.world.pos.y = 60.0f;
            }

            player->actor.world.pos.x *= 10.0f;
            player->actor.world.pos.y -= 3150.0f;
            player->actor.world.pos.y *= 10.0f;
            player->fallStartHeight = player->actor.world.pos.y;
            player->actor.world.pos.z *= 10.0f;

            // If the blue warp exists, and if the player is within 60 units of its center after switching
            // to the "normal" arena, this code will move the player to a fixed point next to the blue warp.
            // This prevents the player from instantly activating the blue warp after switching arenas, and
            // it relies on the blue warp's XZ-coordinates being (0, 0) as they are in Twinmold's boss room.
            if ((sBlueWarp != NULL) && ((SQ(player->actor.world.pos.z) + SQ(player->actor.world.pos.x)) < SQ(60.0f))) {
                player->actor.world.pos.z = 60.0f;
                player->actor.world.pos.x = 60.0f;
            }

            sRedTwinmold->actor.world.pos.x *= 10.0f;
            sRedTwinmold->actor.world.pos.y -= 3150.0f;
            sRedTwinmold->actor.world.pos.y *= 10.0f;
            sRedTwinmold->actor.world.pos.z *= 10.0f;

            sRedTwinmold->unk_0170.x *= 10.0f;
            sRedTwinmold->unk_0170.y -= 3150.0f;
            sRedTwinmold->unk_0170.y *= 10.0f;
            sRedTwinmold->unk_0170.z *= 10.0f;

            for (i = 0; i < ARRAY_COUNT(this->unk_01BC); i++) {
                sRedTwinmold->unk_01BC[i].x *= 10.0f;
                sRedTwinmold->unk_01BC[i].y -= 3150.0f;
                sRedTwinmold->unk_01BC[i].y *= 10.0f;
                sRedTwinmold->unk_01BC[i].z *= 10.0f;
            }

            sRedTwinmold->unk_01B0.x *= 10.0f;
            sRedTwinmold->unk_01B0.y -= 3150.0f;
            sRedTwinmold->unk_01B0.y *= 10.0f;
            sRedTwinmold->unk_01B0.z *= 10.0f;

            sRedTwinmold->unk_017C.y -= 3150.0f;
            sRedTwinmold->unk_017C.y *= 10.0f;
            sRedTwinmold->unk_0188.y -= 3150.0f;
            sRedTwinmold->unk_0188.y *= 10.0f;

            sBlueTwinmold->actor.world.pos.x *= 10.0f;
            sBlueTwinmold->actor.world.pos.y -= 3150.0f;
            sBlueTwinmold->actor.world.pos.y *= 10.0f;
            sBlueTwinmold->actor.world.pos.z *= 10.0f;

            sBlueTwinmold->unk_0170.x *= 10.0f;
            sBlueTwinmold->unk_0170.y -= 3150.0f;
            sBlueTwinmold->unk_0170.y *= 10.0f;
            sBlueTwinmold->unk_0170.z *= 10.0f;

            for (i = 0; i < ARRAY_COUNT(this->unk_01BC); i++) {
                sBlueTwinmold->unk_01BC[i].x *= 10.0f;
                sBlueTwinmold->unk_01BC[i].y -= 3150.0f;
                sBlueTwinmold->unk_01BC[i].y *= 10.0f;
                sBlueTwinmold->unk_01BC[i].z *= 10.0f;
            }

            sBlueTwinmold->unk_01B0.x *= 10.0f;
            sBlueTwinmold->unk_01B0.y -= 3150.0f;
            sBlueTwinmold->unk_01B0.y *= 10.0f;
            sBlueTwinmold->unk_01B0.z *= 10.0f;

            sBlueTwinmold->unk_017C.y -= 3150.0f;
            sBlueTwinmold->unk_017C.y *= 10.0f;
            sBlueTwinmold->unk_0188.y -= 3150.0f;
            sBlueTwinmold->unk_0188.y *= 10.0f;

        } else {
            // In this case, the player is switching from the "normal" arena to the "giant" arena.
            // The "giant" arena sits 3150 units above the "normal" arena, so this moves the player,
            // Twinmold, and the blue warp up and scales their world coordinates downwards to be
            // appropriate for the smaller arena.
            if (sBlueWarp != NULL) {
                sBlueWarp->unk_203 = 1;
                sBlueWarp->unk_204 = 0.1f;
                sBlueWarp->dyna.actor.world.pos.y = 3155.0f;
            }

            player->actor.world.pos.x *= 0.1f;
            player->actor.world.pos.y *= 0.1f;
            player->actor.world.pos.y += 3150.0f;
            player->fallStartHeight = player->actor.world.pos.y;
            player->actor.world.pos.z *= 0.1f;

            sRedTwinmold->actor.world.pos.x *= 0.1f;
            sRedTwinmold->actor.world.pos.y *= 0.1f;
            sRedTwinmold->actor.world.pos.y += 3150.0f;
            sRedTwinmold->actor.world.pos.z *= 0.1f;

            sRedTwinmold->unk_0170.x *= 0.1f;
            sRedTwinmold->unk_0170.y *= 0.1f;
            sRedTwinmold->unk_0170.y += 3150.0f;
            sRedTwinmold->unk_0170.z *= 0.1f;

            for (i = 0; i < ARRAY_COUNT(this->unk_01BC); i++) {
                sRedTwinmold->unk_01BC[i].x *= 0.1f;
                sRedTwinmold->unk_01BC[i].y *= 0.1f;
                sRedTwinmold->unk_01BC[i].y += 3150.0f;
                sRedTwinmold->unk_01BC[i].z *= 0.1f;
            }

            sRedTwinmold->unk_01B0.x *= 0.1f;
            sRedTwinmold->unk_01B0.y *= 0.1f;
            sRedTwinmold->unk_01B0.y += 3150.0f;
            sRedTwinmold->unk_01B0.z *= 0.1f;

            sRedTwinmold->unk_017C.y *= 0.1f;
            sRedTwinmold->unk_017C.y += 3150.0f;
            sRedTwinmold->unk_0188.y *= 0.1f;
            sRedTwinmold->unk_0188.y += 3150.0f;

            sBlueTwinmold->actor.world.pos.x *= 0.1f;
            sBlueTwinmold->actor.world.pos.y *= 0.1f;
            sBlueTwinmold->actor.world.pos.y += 3150.0f;
            sBlueTwinmold->actor.world.pos.z *= 0.1f;

            sBlueTwinmold->unk_0170.x *= 0.1f;
            sBlueTwinmold->unk_0170.y *= 0.1f;
            sBlueTwinmold->unk_0170.y += 3150.0f;
            sBlueTwinmold->unk_0170.z *= 0.1f;

            for (i = 0; i < ARRAY_COUNT(this->unk_01BC); i++) {
                sBlueTwinmold->unk_01BC[i].x *= 0.1f;
                sBlueTwinmold->unk_01BC[i].y *= 0.1f;
                sBlueTwinmold->unk_01BC[i].y += 3150.0f;
                sBlueTwinmold->unk_01BC[i].z *= 0.1f;
            }

            sBlueTwinmold->unk_01B0.x *= 0.1f;
            sBlueTwinmold->unk_01B0.y *= 0.1f;
            sBlueTwinmold->unk_01B0.y += 3150.0f;
            sBlueTwinmold->unk_01B0.z *= 0.1f;

            sBlueTwinmold->unk_017C.y *= 0.1f;
            sBlueTwinmold->unk_017C.y += 3150.0f;
            sBlueTwinmold->unk_0188.y *= 0.1f;
            sBlueTwinmold->unk_0188.y += 3150.0f;
        }

        player->actor.home.pos = player->actor.world.pos;
        player->actor.prevPos = player->actor.world.pos;

        // Kills the existing instance of Twinmold's arena and spawns a new instance with the appropriate
        // scale and at the appropriate height.
        actorIter = play->actorCtx.actorLists[ACTORCAT_BG].first;
        while (actorIter != NULL) {
            if (actorIter->id == ACTOR_BG_INIBS_MOVEBG) {
                Actor_Kill(actorIter);
                break;
            }

            actorIter = actorIter->next;
        }
        {
            f32 arenaPosY = sIsInGiantMode ? 3150.0f : 0.0f;

            Actor_Spawn(&play->actorCtx, play, ACTOR_BG_INIBS_MOVEBG, 0, arenaPosY, 0, 0, 0, 0, sIsInGiantMode);
        }

        // Moves the props in Twinmold's arena and the Heart Container 3150 units up or down
        // so they're in the appropriate arena and scales them accordingly.
        actorIter = play->actorCtx.actorLists[ACTORCAT_BOSS].first;
        while (actorIter != NULL) {
            if ((actorIter->id == ACTOR_EN_TANRON5) || (actorIter->id == ACTOR_ITEM_B_HEART)) {
                if (!sIsInGiantMode) {
                    actorIter->world.pos.y -= 3150.0f;
                    actorIter->world.pos.y *= 10.0f;

                    actorIter->floorHeight -= 3150.0f;
                    actorIter->floorHeight *= 10.0f;

                    scale = 10.0f;
                } else {
                    actorIter->world.pos.y *= 0.1f;
                    actorIter->world.pos.y += 3150.0f;

                    actorIter->floorHeight *= 0.1f;
                    actorIter->floorHeight += 3150.0f;

                    scale = 0.1f;
                }

                actorIter->world.pos.x *= scale;
                actorIter->world.pos.z *= scale;

                actorIter->speed *= scale;

                actorIter->velocity.x *= scale;
                actorIter->velocity.y *= scale;
                actorIter->velocity.z *= scale;

                actorIter->gravity *= scale;
                actorIter->terminalVelocity *= scale;

                actorIter->scale.x *= scale;
                actorIter->scale.y *= scale;
                actorIter->scale.z *= scale;

                if (actorIter->id == ACTOR_ITEM_B_HEART) {
                    ((ItemBHeart*)actorIter)->baseScale *= scale;
                }
            }

            actorIter = actorIter->next;
        }
    }

    switch (this->giantsMaskCsFlashState) {
        case GIANTS_MASK_CS_FLASH_STATE_NOT_STARTED:
            break;

        case GIANTS_MASK_CS_FLASH_STATE_STARTED:
            this->giantsMaskCsFlashAlpha = 0;
            Boss02_FillScreen(play, 255, 255, 255, 0);
            this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_INCREASE_ALPHA;
            Audio_PlaySfx(NA_SE_SY_TRANSFORM_MASK_FLASH);
            // fallthrough
        case GIANTS_MASK_CS_FLASH_STATE_INCREASE_ALPHA:
            this->giantsMaskCsFlashAlpha += 40;
            if (this->giantsMaskCsFlashAlpha >= 400) {
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_DECREASE_ALPHA;
            }

            alpha = this->giantsMaskCsFlashAlpha;
            alpha = CLAMP_MAX(alpha, 255);
            Boss02_SetFillScreenAlpha(play, alpha);
            break;

        case GIANTS_MASK_CS_FLASH_STATE_DECREASE_ALPHA:
            this->giantsMaskCsFlashAlpha -= 40;
            if (this->giantsMaskCsFlashAlpha <= 0) {
                this->giantsMaskCsFlashAlpha = 0;
                this->giantsMaskCsFlashState = GIANTS_MASK_CS_FLASH_STATE_NOT_STARTED;
                Boss02_StopFillScreen(play);
            } else {
                alpha = this->giantsMaskCsFlashAlpha;
                alpha = CLAMP_MAX(alpha, 255);
                Boss02_SetFillScreenAlpha(play, alpha);
            }
            break;
    }

    if ((this->giantsMaskCsState != GIANTS_MASK_CS_STATE_WAITING_FOR_MASK) && (this->subCamId != SUB_CAM_ID_DONE)) {
        Matrix_RotateYS(player->actor.shape.rot.y, MTXMODE_NEW);
        Matrix_MultVecZ(this->subCamDistZFromPlayer, &subCamEyeOffset);

        this->subCamEye.x = player->actor.world.pos.x + subCamEyeOffset.x;
        this->subCamEye.y = player->actor.world.pos.y + subCamEyeOffset.y + this->subCamEyeOffsetY;
        this->subCamEye.z = player->actor.world.pos.z + subCamEyeOffset.z;

        this->subCamAt.x = player->actor.world.pos.x;
        this->subCamAt.y = player->actor.world.pos.y + this->subCamAtOffsetY;
        this->subCamAt.z = player->actor.world.pos.z;

        this->subCamUpRotZ = Math_SinS(this->giantsMaskCsTimer * 1512) * this->subCamUpRotZScale;
        Matrix_RotateZF(this->subCamUpRotZ, MTXMODE_APPLY);
        Matrix_MultVecY(1.0f, &this->subCamUp);
        Play_SetCameraAtEyeUp(play, this->subCamId, &this->subCamAt, &this->subCamEye, &this->subCamUp);
        ShrinkWindow_Letterbox_SetSizeTarget(27);
    }
}

void func_809DEAC4(Boss02* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    Boss02* sp68;
    Vec3f subCamEye;
    f32 subCamEyeOffset = 0.0f;

    this->unk_1D1C++;
    if (this->unk_0194 == 0) {
        sp68 = sRedTwinmold;
    } else {
        sp68 = sBlueTwinmold;
    }

    switch (this->unk_1D20) {
        case 0:
            break;

        case 1:
            if (CHECK_WEEKEVENTREG(WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE) ||
                ((u32)(KREG(13) + 15) >= this->unk_1D1C)) {
                break;
            }
            Cutscene_StartManual(play, &play->csCtx);
            this->subCamId = Play_CreateSubCamera(play);
            Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
            Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
            this->unk_1D20 = 2;
            this->unk_1D1C = 0;

        case 2:
            player->actor.shape.rot.y = -0x8000;
            player->actor.world.rot.y = player->actor.shape.rot.y;
            this->subCamEye.x = player->actor.world.pos.x - 20.0f;
            this->subCamEye.y = (Player_GetHeight(player) + player->actor.world.pos.y) - 29.0f;
            this->subCamEye.z = player->actor.world.pos.z - 50;

            this->subCamAt.x = player->actor.world.pos.x;
            this->subCamAt.y = (Player_GetHeight(player) + player->actor.world.pos.y) - 17.0f;
            this->subCamAt.z = player->actor.world.pos.z;
            if (this->unk_1D1C >= 30) {
                if (this->unk_1D1C == 30) {
                    Rumble_Override(0.0f, 50, 200, 1);
                }
                this->unk_0150 += 0x4000;
                subCamEyeOffset = (Math_SinS(this->unk_0150) * (BREG(19) + 5)) * 0.1f;
                Matrix_RotateZF(Math_SinS(this->unk_1D1C * 0x3000) * ((KREG(28) * 0.001f) + 0.017f), MTXMODE_NEW);
                Matrix_MultVecY(1.0f, &this->subCamUp);
                Audio_PlaySfx_2(NA_SE_EV_EARTHQUAKE_LAST - SFX_FLAG);
            }

            if (this->unk_1D1C == 20) {
                this->unk_1D7E = 100;
            }

            if (this->unk_1D1C == 45) {
                Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_21);
                sTwinmoldMusicStartTimer = KREG(91) + 43;
            }

            if (this->unk_1D1C == 85) {
                sRedTwinmold->unk_0144 = 11;
                sRedTwinmold->unk_014C = 0;
            }

            if (this->unk_1D1C == 92) {
                Rumble_Override(0.0f, 255, 30, 100);
            }

            if (this->unk_1D1C == 100) {
                Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_115);
            }

            if (this->unk_1D1C == 112) {
                this->unk_1D20 = 10;
                this->unk_1D1C = 0;
            }
            break;

        case 10:
            if (this->unk_1D1C < 310) {
                this->subCamAtNext = sp68->actor.world.pos;
                this->subCamEye.x = (sp68->actor.world.pos.x * 50.0f) * 0.001f;
                this->subCamEye.y = (sp68->actor.world.pos.y * 50.0f) * 0.001f;
                if (this->subCamEye.y < 100.0f) {
                    this->subCamEye.y = 100.0f;
                }
                this->subCamEye.z = (sp68->actor.world.pos.z * 100) * 0.001f;
            }

            Math_ApproachF(&this->subCamAt.x, this->subCamAtNext.x, 0.1f, 3000.0f);
            Math_ApproachF(&this->subCamAt.y, this->subCamAtNext.y, 0.1f, 3000.0f);
            Math_ApproachF(&this->subCamAt.z, this->subCamAtNext.z, 0.1f, 3000.0f);

            if (this->unk_1D1C == 100) {
                sBlueTwinmold->unk_0144 = 11;
                sBlueTwinmold->unk_014C = 0;
                this->unk_0194 = 1;
            }

            if (this->unk_1D1C == (u32)(KREG(92) + 125)) {
                TitleCard_InitBossName(&play->state, &play->actorCtx.titleCtx,
                                       Lib_SegmentedToVirtual(gTwinmoldTitleCardTex), 160, 180, 128, 40);
            }

            if (this->unk_1D1C == (u32)(BREG(27) + 335)) {
                func_80169AFC(play, this->subCamId, 0);
                this->subCamId = SUB_CAM_ID_DONE;
                Cutscene_StopManual(play, &play->csCtx);
                Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
                this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
                this->unk_1D20 = 0;
                sRedTwinmold->unk_0144 = sBlueTwinmold->unk_0144 = 3;
                sRedTwinmold->unk_0146[0] = sBlueTwinmold->unk_0146[0] = 60;
                SET_EVENTINF(EVENTINF_INTRO_CS_WATCHED_TWINMOLD);
            }
            break;

        case 100:
            if (CutsceneManager_GetCurrentCsId() == CS_ID_NONE) {
                Cutscene_StartManual(play, &play->csCtx);
                this->subCamId = Play_CreateSubCamera(play);
                Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STATUS_WAIT);
                Play_ChangeCameraStatus(play, this->subCamId, CAM_STATUS_ACTIVE);
                this->unk_1D20 = 101;
                this->unk_1D1C = 0;
                this->subCamAtVel = 1.0f;
                this->subCamAt = sp68->actor.world.pos;
            } else {
                break;
            }

        case 101:
        case 102:
        case_10x:
            if ((this->unk_1D20 == 101) || (this->unk_1D20 == 103)) {
                this->subCamAtNext = sp68->actor.world.pos;
            } else {
                this->subCamAtNext = sp68->unk_147C[sp68->unk_1678];
            }

            this->subCamEye.x = player->actor.world.pos.x;
            this->subCamEye.y = player->actor.world.pos.y + 100.0f;
            if (!sIsInGiantMode) {
                if (this->subCamEye.y < 100.0f) {
                    this->subCamEye.y = 100.0f;
                }
            } else if (this->subCamEye.y < 3160.0f) {
                this->subCamEye.y = 3160.0f;
            }

            if (this->subCamAtNext.y < (100.0f * sGiantModeScaleFactor)) {
                this->subCamAtNext.y = (100.0f * sGiantModeScaleFactor);
            }

            this->subCamEye.z = player->actor.world.pos.z;
            Math_ApproachF(&this->subCamAt.x, this->subCamAtNext.x, 0.3f, this->subCamAtVel * 500.0f);
            Math_ApproachF(&this->subCamAt.y, this->subCamAtNext.y, 0.3f, this->subCamAtVel * 500.0f);
            Math_ApproachF(&this->subCamAt.z, this->subCamAtNext.z, 0.3f, this->subCamAtVel * 500.0f);
            Math_ApproachF(&this->subCamAtVel, 1.0f, 1.0f, 0.02f);
            break;

        case 103:
            this->unk_0150 += 0x4000;
            subCamEyeOffset = Math_SinS(this->unk_0150);
            subCamEyeOffset = (subCamEyeOffset * this->unk_0146[0]) * 1.5f;
            if (this->unk_1D1C == 30) {
                func_80169AFC(play, this->subCamId, 0);
                this->subCamId = SUB_CAM_ID_DONE;
                Cutscene_StopManual(play, &play->csCtx);
                Player_SetCsActionWithHaltedActors(play, &this->actor, PLAYER_CSACTION_END);
                this->unk_1D20 = 0;
                this->actor.flags |= ACTOR_FLAG_ATTENTION_ENABLED;
                sp68->unk_0144 = 10;
                if ((sRedTwinmold->unk_0144 >= 10) && (sBlueTwinmold->unk_0144 >= 10)) {
                    f32 phi_f0;

                    this->unk_1D7E = 0;
                    if (!sIsInGiantMode) {
                        Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_B_HEART, 0.0f, 30.0f, -150.0f, 0, 1, 0,
                                    BHEART_PARAM_NORMAL);
                        phi_f0 = 60.0f;
                    } else {
                        Actor_Spawn(&play->actorCtx, play, ACTOR_ITEM_B_HEART, 0.0f, 3153.0f, -15.0f, 0, 1, 0,
                                    BHEART_PARAM_SMALL);
                        phi_f0 = 3155.0f;
                    }
                    sBlueWarp = (DoorWarp1*)Actor_SpawnAsChild(&play->actorCtx, &this->actor, play, ACTOR_DOOR_WARP1,
                                                               0.0f, phi_f0, 0.0f, 0, 0, 0, ENDOORWARP1_FF_1);

                    if (!sIsInGiantMode) {
                        sBlueWarp->unk_203 = 0;
                        sBlueWarp->unk_204 = 1.0f;
                    } else {
                        sBlueWarp->unk_203 = 1;
                        sBlueWarp->unk_204 = 0.1f;
                    }
                }
            }
            goto case_10x;
    }

    if ((this->unk_1D20 != 0) && (this->subCamId != SUB_CAM_ID_DONE)) {
        subCamEye = this->subCamEye;
        subCamEye.y += subCamEyeOffset * sGiantModeScaleFactor;
        Play_SetCameraAtEyeUp(play, this->subCamId, &this->subCamAt, &subCamEye, &this->subCamUp);
        this->subCamUp.z = this->subCamUp.x = 0.0f;
        this->subCamUp.y = 1.0f;
        ShrinkWindow_Letterbox_SetSizeTarget(27);
    }
}

void Boss02_Reset(void) {
    sCanSkipMaskOnCs = false;
    sCanSkipMaskOffCs = false;
    sIsInGiantMode = false;
    sRedTwinmold = NULL;
    sBlueTwinmold = NULL;
    sTwinmoldBattleHandler = NULL;
    sTwinmoldMusicStartTimer = 0;
    sBlueWarp = NULL;
    sGiantModeScaleFactor = 1.0f;
    sBoTmShotCount = 0;
    sBoTmPatchCount = 0;

    for (int i = 0; i < TWINMOLD_EFFECT_COUNT; i++) {
        sTwinmoldEffects[i].type = TWINMOLD_EFFECT_NONE;
        sTwinmoldEffects[i].epoch = 0;
    }
}
