#ifndef Z_EN_M_THUNDER_H
#define Z_EN_M_THUNDER_H

#include "global.h"

struct EnMThunder;

#define ENMTHUNDER_GET_TYPE(thisx) ((thisx)->params & 0xFF)
#define ENMTHUNDER_GET_MAGIC_COST(thisx) (((thisx)->params & 0xFF00) >> 8)

typedef enum {
    /* 0x00 */ ENMTHUNDER_TYPE_KOKIRI_SWORD,
    /* 0x01 */ ENMTHUNDER_TYPE_RAZOR_SWORD,
    /* 0x02 */ ENMTHUNDER_TYPE_GILDED_SWORD,
    /* 0x03 */ ENMTHUNDER_TYPE_GREAT_FAIRYS_SWORD,
    /* 0x80 */ ENMTHUNDER_TYPE_UNK = 0x80
} EnMThunderType;

typedef void (*EnMThunderActionFunc)(struct EnMThunder*, PlayState*);

typedef struct EnMThunder {
    /* 0x000 */ Actor actor;
    /* 0x144 */ ColliderCylinder collider;
    /* 0x190 */ LightNode* lightNode;
    /* 0x194 */ LightInfo lightInfo;
    /* 0x1A4 */ f32 lightColorFrac;
    /* 0x1A8 */ f32 alphaFrac;
    /* 0x1AC */ f32 scroll;
    /* 0x1B0 */ f32 unk1B0;
    /* 0x1B4 */ f32 adjustLightsArg1;
    /* 0x1B8 */ EnMThunderActionFunc actionFunc;
    /* 0x1BC */ u16 timer;
    /* 0x1BE */ u8 subtype;
    /* 0x1BF */ u8 type;
    /* 0x1C0 */ u8 chargingAlpha;
    /* 0x1C1 */ u8 scaleTarget;
    /* 0x1C2 */ u8 isCharging;
    // Sword projectiles (Enhancements > Items > Sword Projectiles), only used when ENMTHUNDER_IS_SWORD_SHOT
    s16 shotTimer;
    s16 shotCooldown;
    s16 shotSpin;
    u8 shotKind;
} EnMThunder;

typedef enum EnMThunderShotKind {
    /* 0 */ ENMTHUNDER_SHOT_GILDED,      // golden crescent: one hit, like the Master Sword beam of Ocarina of Time
    /* 1 */ ENMTHUNDER_SHOT_GREAT_FAIRY, // spinning ring: slower, bigger, goes through enemies
    /* 2 */ ENMTHUNDER_SHOT_DEITY,       // deity orb: fastest, bursts on impact
    /* 3 */ ENMTHUNDER_SHOT_DEITY_BURST, // the burst left by a deity orb
    /* 4 */ ENMTHUNDER_SHOT_MAX
} EnMThunderShotKind;

// An En_M_Thunder spawned with these params is a sword projectile instead of a spin attack. Vanilla params are
// (magic cost << 8) | sword type, which never reach these values.
#define ENMTHUNDER_SWORD_SHOT_PARAMS(kind) (0x7B10 + (kind))
#define ENMTHUNDER_IS_SWORD_SHOT(thisx) \
    (((thisx)->params >= ENMTHUNDER_SWORD_SHOT_PARAMS(0)) && \
     ((thisx)->params < ENMTHUNDER_SWORD_SHOT_PARAMS(ENMTHUNDER_SHOT_MAX)))

// Called by Player when a sword swing becomes active: fires the held sword's projectile if the conditions are met.
void EnMThunder_TrySwordShot(PlayState* play, Player* player);
// True when the Fierce Deity's vanilla sword beam is replaced by the deity orb.
s32 EnMThunder_DeityShotReplacesBeam(void);

#endif // Z_EN_M_THUNDER_H
