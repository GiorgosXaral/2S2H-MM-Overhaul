#ifndef Z_BOSS_02_H
#define Z_BOSS_02_H

#include "global.h"
#include "2s2h/Enhancements/BossOverhaul/BossOverhaul.h"
#include "objects/object_boss02/object_boss02.h"

struct Boss02;

#define TWINMOLD_GET_TYPE(thisx) ((thisx)->params)

typedef void (*Boss02ActionFunc)(struct Boss02*, PlayState*);

// Boss Overhaul: an instance of Boss02 spawned with these params is one of the twin dragons' projectiles
#define BO_TM_SHOT_PARAMS 151

typedef enum Boss02BoElement {
    /* 0 */ BO_TM_FIRE, // Red Twinmold
    /* 1 */ BO_TM_ICE   // Blue Twinmold
} Boss02BoElement;

typedef enum Boss02BoShotKind {
    /* 0 */ BO_TM_SHOT_SPARK,   // visual only: a ball spat into the sky to call down a storm
    /* 1 */ BO_TM_SHOT_BREATH,  // one ball of a breath stream
    /* 2 */ BO_TM_SHOT_LOB,     // thrown out of an eruption
    /* 3 */ BO_TM_SHOT_FALLING, // a meteor or a giant icicle: its landing spot is marked, then it drops from the sky
    /* 4 */ BO_TM_SHOT_PATCH    // burning or frozen ground left behind by the big attacks
} Boss02BoShotKind;

typedef struct Boss02BoShot {
    ColliderCylinder collider;
    Vec3f target;    // falling: the landing spot
    f32 size;        // grows from 0 to 1
    f32 gravity;     // added to velocity.y every frame
    f32 hitRadius;
    s16 kind;        // Boss02BoShotKind
    s16 phase;
    s16 timer;       // counts up
    s16 life;        // counts down
    s16 spin;
    s16 hitCooldown; // patches: frames before it can hurt again
    u8 element;      // Boss02BoElement
    u8 giant;        // spawned in the Giant's Mask arena
    u8 leavesPatch;  // burns / freezes the ground where it lands
} Boss02BoShot;

typedef struct {
    /* 0x00 */ Vec3f pos;
    /* 0x0C */ Vec3f velocity;
    /* 0x18 */ Vec3f accel;
    /* 0x24 */ u8 type;
    /* 0x26 */ s16 timer;
    /* 0x28 */ UNK_TYPE1 unk_28[0x4];
    /* 0x2C */ s16 alpha;
    /* 0x2E */ s16 rotX;
    /* 0x30 */ s16 rotY;
    /* 0x34 */ f32 scale;
    /* 0x38 */ f32 targetScale;
    // 2S2H [Port]
    /*      */ s16 epoch; // Tracks when an effect is spawned for use with interpolation skipping
} TwinmoldEffect; // size = 0x3C

typedef enum {
    /* 0 */ TWINMOLD_EFFECT_NONE,
    /* 1 */ TWINMOLD_EFFECT_SAND,       // The sand kicked up when Twinmold touches the ground
    /* 2 */ TWINMOLD_EFFECT_BLACK_DUST, // The dust that appears when either Twinmold or the player destroys a ruin
    /* 3 */ TWINMOLD_EFFECT_FRAGMENT,   // The fragments that fly off when the parts of Twinmold explode
    /* 4 */ TWINMOLD_EFFECT_FLASH       // The flashes of light that appear when the parts of Twinmold explode
} TwinmoldEffectType;

#define TWINMOLD_EFFECT_COUNT 150

typedef enum {
    /*   0 */ TWINMOLD_TYPE_RED,
    /*  35 */ TWINMOLD_TYPE_BLUE = 35,
    /* 100 */ TWINMOLD_TYPE_TAIL = 100,
    /* 200 */ TWINMOLD_TYPE_BATTLE_HANDLER = 200
} TwinmoldType;

typedef struct Boss02 {
    /* 0x0000 */ Actor actor;
    /* 0x0144 */ u8 unk_0144;
    /* 0x0146 */ s16 unk_0146[3];
    /* 0x014C */ s16 unk_014C;
    /* 0x014E */ s16 unk_014E;
    /* 0x0150 */ s16 unk_0150;
    /* 0x0152 */ s16 unk_0152;
    /* 0x0154 */ s16 unk_0154;
    /* 0x0156 */ s16 unk_0156;
    /* 0x0158 */ s16 unk_0158;
    /* 0x015A */ s16 unk_015A;
    /* 0x015C */ u8 unk_015C;
    /* 0x015D */ UNK_TYPE1 unk015D[0x7];
    /* 0x0164 */ f32 unk_0164;
    /* 0x0168 */ f32 unk_0168;
    /* 0x016C */ s16 unk_016C;
    /* 0x0170 */ Vec3f unk_0170;
    /* 0x017C */ Vec3f unk_017C;
    /* 0x0188 */ Vec3f unk_0188;
    /* 0x0194 */ u8 unk_0194;
    /* 0x0195 */ u8 unk_0195;
    /* 0x0196 */ s16 unk_0196;
    /* 0x0198 */ s16 unk_0198;
    /* 0x019A */ s16 unk_019A;
    /* 0x019C */ f32 unk_019C;
    /* 0x01A0 */ f32 unk_01A0;
    /* 0x01A4 */ s16 unk_01A4;
    /* 0x01A8 */ f32 unk_01A8;
    /* 0x01AC */ f32 giantModeScaleFactor;
    /* 0x01B0 */ Vec3f unk_01B0;
    /* 0x01BC */ Vec3f unk_01BC[200];
    /* 0x0B1C */ Vec3f unk_0B1C[200];
    /* 0x147C */ Vec3f unk_147C[23];
    /* 0x1590 */ SkelAnime skelAnime;
    /* 0x15D4 */ Vec3s jointTable[TWINMOLD_HEAD_LIMB_MAX];
    /* 0x1622 */ Vec3s morphTable[TWINMOLD_HEAD_LIMB_MAX];
    /* 0x1670 */ Boss02ActionFunc actionFunc;
    /* 0x1674 */ struct Boss02* otherTwinmold; // points to the other's instance
    /* 0x1678 */ s32 unk_1678;
    /* 0x167C */ Vec3f unk_167C;
    /* 0x1688 */ ColliderJntSph colliderSphere1;
    /* 0x16A8 */ ColliderJntSphElement colliderSphere1Elements[22];
    /* 0x1C28 */ ColliderJntSph colliderSphere2;
    /* 0x1C48 */ ColliderJntSphElement colliderSphere2Elements[2];
    /* 0x1CC8 */ ColliderCylinder colliderCylinder;
    /* 0x1D14 */ u32 giantsMaskCsTimer;
    /* 0x1D18 */ s16 giantsMaskCsState;
    /* 0x1D1A */ s16 unk_1D1A;
    /* 0x1D1C */ u32 unk_1D1C;
    /* 0x1D20 */ s16 unk_1D20;
    /* 0x1D22 */ s16 subCamId;
    /* 0x1D24 */ Vec3f subCamEye;
    /* 0x1D30 */ Vec3f subCamAt;
    /* 0x1D3C */ Vec3f subCamUp;
    /* 0x1D48 */ Vec3f subCamAtNext;
    /* 0x1D54 */ f32 subCamUpRotZ;
    /* 0x1D58 */ f32 subCamUpRotZScale;
    /* 0x1D5C */ f32 subCamAtVel;
    /* 0x1D60 */ UNK_TYPE1 unk1D60[0x4];
    /* 0x1D64 */ f32 subCamDistZFromPlayer;
    /* 0x1D68 */ f32 subCamEyeOffsetY;
    /* 0x1D6C */ f32 subCamAtOffsetY;
    /* 0x1D70 */ f32 playerScale;
    /* 0x1D74 */ f32 fogNear;
    /* 0x1D78 */ u8 giantsMaskCsFlashState;
    /* 0x1D7A */ s16 giantsMaskCsFlashAlpha;
    /* 0x1D7C */ s16 unk_1D7C;
    /* 0x1D7E */ s16 unk_1D7E;
    // Boss Overhaul: the twin dragons
    s16 boWindupTimer;    // > 0 while gathering a breath
    s16 boBreathTimer;    // > 0 while breathing
    s16 boBreathCooldown;
    s16 boEruptCooldown;
    s16 boRainCooldown;
    u8 boBreathElement;   // element of the current / next breath (the survivor alternates)
    u8 boRainFlip;
    u8 boLastGiantMode;
    f32 boGlow;           // 0..1: the jaws glow and light up the arena while a breath gathers and pours out
    f32 boSpeedScale;     // flying speed multiplier: the dragon slows down while it breathes
    Vec3f boBreathTarget;
    Vec3f boMouthPos;     // from the last draw
    LightNode* boLightNode;
    LightInfo boLightInfo;
    Boss02BoShot boShot;  // state of an instance spawned as a projectile (BO_TM_SHOT_PARAMS)
} Boss02; // size = 0x1D80 (vanilla)

#endif // Z_BOSS_02_H
