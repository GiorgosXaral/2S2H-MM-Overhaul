/**
 * Miniboss & Enemy Overhaul: encounters.
 *
 *  - Companions: some enemies placed in a room bring one or two of their own kind along, and in a few regions a
 *    creature that belongs there (Keese with Chuchus on the night fields, Dragonflies over the swamp's Octoroks, Ice
 *    Keese with Eenos in the mountains, Bubbles with Ikana's Stalchildren).
 *  - Tag-alongs: the game's random-encounter spawners sometimes send an extra: another Dragonfly in the swamp, and a
 *    second Garo who drops in behind Link during a Garo ambush.
 *  - Elites: about one enemy in twenty is bigger, sparkles gold, has twice the health and drops something better.
 *  - Final Day frenzy: on the third day more companions show up, and more still (and more elites) after midnight.
 *
 * Nothing is added to boss rooms, minigames, the Moon, Spider Houses, grottos or Secret Shrine, nor (in dungeons) to a
 * room whose door or chest opens once every enemy is defeated. Companions and tag-alongs are ordinary enemies that
 * don't count towards any spawner, never bring companions of their own, and are placed on open floor in sight of the
 * enemy that brought them.
 */

#include "EnemyOverhaul.h"
#include "z64door.h"
#include "overlays/actors/ovl_Door_Shutter/z_door_shutter.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Firefly/z_en_firefly.h"
#include "overlays/actors/ovl_En_Grasshopper/z_en_grasshopper.h"
#include "overlays/actors/ovl_En_Crow/z_en_crow.h"
#include "overlays/actors/ovl_En_Neo_Reeba/z_en_neo_reeba.h"
#include "overlays/actors/ovl_En_Jso/z_en_jso.h"

typedef enum EoRegion {
    /* 0 */ EO_REGION_NONE, // not touched at all
    /* 1 */ EO_REGION_FIELD,
    /* 2 */ EO_REGION_SWAMP,
    /* 3 */ EO_REGION_MOUNTAIN,
    /* 4 */ EO_REGION_OCEAN,
    /* 5 */ EO_REGION_IKANA,
} EoRegion;

#define EO_REGION_ANY 0xFF

typedef struct EoSceneInfo {
    s16 sceneId;
    u8 region;
    u8 isDungeon;
} EoSceneInfo;

static EoSceneInfo sEoScenes[] = {
    { SCENE_00KEIKOKU, EO_REGION_FIELD, false },         // Termina Field
    { SCENE_24KEMONOMITI, EO_REGION_FIELD, false },      // Road to Southern Swamp
    { SCENE_ROMANYMAE, EO_REGION_FIELD, false },         // Milk Road
    { SCENE_20SICHITAI, EO_REGION_SWAMP, false },        // Southern Swamp (poisoned)
    { SCENE_20SICHITAI2, EO_REGION_SWAMP, false },       // Southern Swamp (clean)
    { SCENE_21MITURINMAE, EO_REGION_SWAMP, false },      // Woodfall
    { SCENE_26SARUNOMORI, EO_REGION_SWAMP, false },      // Woods of Mystery
    { SCENE_MITURIN, EO_REGION_SWAMP, true },            // Woodfall Temple
    { SCENE_10YUKIYAMANOMURA, EO_REGION_MOUNTAIN, false },  // Mountain Village (winter)
    { SCENE_10YUKIYAMANOMURA2, EO_REGION_MOUNTAIN, false }, // Mountain Village (spring)
    { SCENE_11GORONNOSATO, EO_REGION_MOUNTAIN, false },     // Goron Village (winter)
    { SCENE_11GORONNOSATO2, EO_REGION_MOUNTAIN, false },    // Goron Village (spring)
    { SCENE_12HAKUGINMAE, EO_REGION_MOUNTAIN, false },      // Snowhead
    { SCENE_13HUBUKINOMITI, EO_REGION_MOUNTAIN, false },    // Path to Snowhead
    { SCENE_14YUKIDAMANOMITI, EO_REGION_MOUNTAIN, false },  // Path to Mountain Village
    { SCENE_17SETUGEN, EO_REGION_MOUNTAIN, false },         // Twin Islands (winter)
    { SCENE_17SETUGEN2, EO_REGION_MOUNTAIN, false },        // Twin Islands (spring)
    { SCENE_HAKUGIN, EO_REGION_MOUNTAIN, true },            // Snowhead Temple
    { SCENE_30GYOSON, EO_REGION_OCEAN, false },             // Great Bay Coast
    { SCENE_31MISAKI, EO_REGION_OCEAN, false },             // Zora Cape
    { SCENE_35TAKI, EO_REGION_OCEAN, false },               // Waterfall Rapids
    { SCENE_TORIDE, EO_REGION_OCEAN, false },               // Pirates' Fortress exterior
    { SCENE_KAIZOKU, EO_REGION_OCEAN, true },               // Pirates' Fortress
    { SCENE_PIRATE, EO_REGION_OCEAN, true },                // Pirates' Fortress interior
    { SCENE_SEA, EO_REGION_OCEAN, true },                   // Great Bay Temple
    { SCENE_IKANAMAE, EO_REGION_IKANA, false },             // Road to Ikana
    { SCENE_IKANA, EO_REGION_IKANA, false },                // Ikana Canyon
    { SCENE_BOTI, EO_REGION_IKANA, false },                 // Ikana Graveyard
    { SCENE_F40, EO_REGION_IKANA, false },                  // Stone Tower
    { SCENE_F41, EO_REGION_IKANA, false },                  // Stone Tower (inverted)
    { SCENE_CASTLE, EO_REGION_IKANA, true },                // Ancient Castle of Ikana
    { SCENE_REDEAD, EO_REGION_IKANA, true },                // Beneath the Well
    { SCENE_HAKASHITA, EO_REGION_IKANA, true },             // Beneath the Graveyard
    { SCENE_INISIE_N, EO_REGION_IKANA, true },              // Stone Tower Temple
    { SCENE_INISIE_R, EO_REGION_IKANA, true },              // Stone Tower Temple (inverted)
};

static EoSceneInfo* EoScene_Get(PlayState* play) {
    s32 i;

    for (i = 0; i < ARRAY_COUNT(sEoScenes); i++) {
        if (sEoScenes[i].sceneId == play->sceneId) {
            return &sEoScenes[i];
        }
    }
    return NULL;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Final Day frenzy
 * --------------------------------------------------------------------------------------------------------------- */

// 0 normally, 1 on the Final Day, 2 on the final night after midnight.
static s32 Eo_FrenzyLevel(void) {
    if (!EO_FRENZY_ON() || (CURRENT_DAY != 3)) {
        return 0;
    }
    return (CURRENT_TIME < CLOCK_TIME(6, 0)) ? 2 : 1;
}

static s32 Eo_IsNight(void) {
    return (CURRENT_TIME >= CLOCK_TIME(18, 0)) || (CURRENT_TIME < CLOCK_TIME(6, 0));
}

static s32 Eo_Roll(s32 percent) {
    s32 frenzy = Eo_FrenzyLevel();

    percent *= (frenzy + 1);
    percent = CLAMP_MAX(percent, 90);
    return Rand_ZeroFloat(100.0f) < percent;
}

/* ---------------------------------------------------------------------------------------------------------------
 * Companions
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_PARAMS_COPY -1

typedef enum EoPlacement {
    /* 0 */ EO_PLACE_GROUND, // on the floor, about level with the enemy
    /* 1 */ EO_PLACE_AIR,    // at the enemy's height (plus the rule's lift), above some floor
    /* 2 */ EO_PLACE_WATER,  // on the same water surface as the enemy
} EoPlacement;

typedef struct EoCompanionRule {
    s16 triggerId;
    s16 companionId;
    s32 params; // EO_PARAMS_COPY to copy the trigger's
    u8 region;  // EO_REGION_ANY or one region
    u8 chance;  // percent, before the frenzy
    u8 minCount;
    u8 maxCount;
    u8 placement;
    u8 nightOnly;
    s16 lift;
} EoCompanionRule;

static EoCompanionRule sEoCompanionRules[] = {
    // Their own kind
    { ACTOR_EN_FIREFLY, ACTOR_EN_FIREFLY, EO_PARAMS_COPY, EO_REGION_ANY, 35, 1, 2, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_SLIME, ACTOR_EN_SLIME, EO_PARAMS_COPY, EO_REGION_ANY, 35, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_MKK, ACTOR_EN_MKK, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_SKB, ACTOR_EN_SKB, EO_PARAMS_COPY, EO_REGION_ANY, 35, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_GRASSHOPPER, ACTOR_EN_GRASSHOPPER, EO_PARAMS_COPY, EO_REGION_ANY, 20, 1, 1, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_BB, ACTOR_EN_BB, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_BBFALL, ACTOR_EN_BBFALL, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_CROW, ACTOR_EN_CROW, EO_PARAMS_COPY, EO_REGION_ANY, 40, 1, 2, EO_PLACE_AIR, false, 0 },
    { ACTOR_EN_NEO_REEBA, ACTOR_EN_NEO_REEBA, EO_PARAMS_COPY, EO_REGION_ANY, 40, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_DEKUBABA, ACTOR_EN_DEKUBABA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_KAREBABA, ACTOR_EN_KAREBABA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_OKUTA, ACTOR_EN_OKUTA, EO_PARAMS_COPY, EO_REGION_ANY, 25, 1, 1, EO_PLACE_WATER, false, 0 },
    { ACTOR_EN_RAT, ACTOR_EN_RAT, EO_PARAMS_COPY, EO_REGION_ANY, 30, 1, 2, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_SNOWMAN, ACTOR_EN_SNOWMAN, EO_PARAMS_COPY, EO_REGION_ANY, 20, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_KAME, ACTOR_EN_KAME, EO_PARAMS_COPY, EO_REGION_ANY, 20, 1, 1, EO_PLACE_GROUND, false, 0 },
    { ACTOR_EN_PP, ACTOR_EN_PP, EO_PARAMS_COPY, EO_REGION_ANY, 15, 1, 1, EO_PLACE_GROUND, false, 0 },
    // Creatures that belong where they are found
    { ACTOR_EN_SLIME, ACTOR_EN_FIREFLY, KEESE_NORMAL_FLY, EO_REGION_FIELD, 20, 1, 2, EO_PLACE_AIR, true, 60 },
    { ACTOR_EN_OKUTA, ACTOR_EN_GRASSHOPPER, DRAGONFLY_TYPE_NORMAL, EO_REGION_SWAMP, 25, 1, 1, EO_PLACE_AIR, false, 80 },
    { ACTOR_EN_SNOWMAN, ACTOR_EN_FIREFLY, KEESE_ICE_FLY, EO_REGION_MOUNTAIN, 20, 1, 2, EO_PLACE_AIR, false, 80 },
    { ACTOR_EN_SKB, ACTOR_EN_BB, 0xFF00, EO_REGION_IKANA, 20, 1, 1, EO_PLACE_AIR, false, 50 },
};

// Variants of an enemy that must not be copied (perched, giant, fragments, projectiles, special-purpose ones).
static s32 EoCompanion_CanCopy(Actor* actor) {
    s32 type = actor->params & 0xFF;

    switch (actor->id) {
        case ACTOR_EN_FIREFLY:
            return KEESE_GET_MAIN_TYPE(actor) != KEESE_NORMAL_PERCH;
        case ACTOR_EN_GRASSHOPPER:
            return (actor->params == DRAGONFLY_TYPE_NORMAL) || (actor->params == DRAGONFLY_TYPE_UNUSED_NORMAL) ||
                   (actor->params == DRAGONFLY_TYPE_WOODFALL);
        case ACTOR_EN_CROW:
            return actor->params == GUAY_TYPE_NORMAL;
        case ACTOR_EN_NEO_REEBA:
            return !EN_NEO_REEBA_IS_LARGE(actor);
        case ACTOR_EN_KAREBABA:
            return type <= 1;
        case ACTOR_EN_OKUTA:
            return type == 0; // red Octorok, not a projectile
        case ACTOR_EN_SNOWMAN:
            return type <= 1; // small or large Eeno, not a split piece or a snowball
        case ACTOR_EN_PP:
            return type <= 2; // Hiploops, not their fragments
        case ACTOR_EN_MKK:
            return (actor->params & 2) == 0; // not a Boe split off from a Big Boe
        default:
            return true;
    }
}

/**
 * True when defeating every enemy in `room` opens a door, reveals a chest or does something else, so extra enemies
 * are kept out of it.
 */
static s32 EoRoom_HasClearReward(PlayState* play, s8 room) {
    s32 category;
    Actor* actor;

    for (category = 0; category < ACTORCAT_MAX; category++) {
        for (actor = play->actorCtx.actorLists[category].first; actor != NULL; actor = actor->next) {
            switch (actor->id) {
                case ACTOR_DOOR_SHUTTER:
                    if (DOORSHUTTER_GET_TYPE(actor) == DOORSHUTTER_TYPE_1) {
                        u16 index = DOOR_GET_TRANSITION_ID(actor);

                        if ((index >= play->transitionActors.count) ||
                            (play->transitionActors.list[index].sides[0].room == room) ||
                            (play->transitionActors.list[index].sides[1].room == room)) {
                            return true;
                        }
                    }
                    break;

                case ACTOR_EN_BOX:
                    if ((actor->room == room) && ((ENBOX_GET_TYPE(actor) == ENBOX_TYPE_BIG_ROOM_CLEAR) ||
                                                  (ENBOX_GET_TYPE(actor) == ENBOX_TYPE_SMALL_ROOM_CLEAR))) {
                        return true;
                    }
                    break;

                case ACTOR_BG_IKANA_SHUTTER:
                case ACTOR_BG_HAKA_CURTAIN:
                case ACTOR_BG_HAKA_TOMB:
                    if ((actor->room == room) || (actor->room < 0)) {
                        return true;
                    }
                    break;

                default:
                    break;
            }
        }
    }
    return false;
}

/**
 * Finds a spot 90-170 units from `anchor` with nothing in between. Returns false when every try is blocked.
 */
static s32 EoCompanion_FindSpot(PlayState* play, Actor* anchor, u8 placement, s16 lift, Vec3f* out) {
    s32 try;

    for (try = 0; try < 6; try++) {
        s16 yaw = (s16)Rand_Next();
        f32 dist = 90.0f + Rand_ZeroFloat(80.0f);
        Vec3f from = anchor->world.pos;
        Vec3f spot = anchor->world.pos;
        Vec3f hitPos;
        CollisionPoly* poly;
        s32 bgId;
        f32 floorY;

        spot.x += Math_SinS(yaw) * dist;
        spot.z += Math_CosS(yaw) * dist;
        from.y += 25.0f;
        spot.y += 25.0f;

        if (BgCheck_EntityLineTest1(&play->colCtx, &from, &spot, &hitPos, &poly, true, false, false, true, &bgId)) {
            continue;
        }
        spot.y -= 25.0f;

        if (placement == EO_PLACE_WATER) {
            f32 surfaceY;
            WaterBox* waterBox;

            if (!WaterBox_GetSurface1(play, &play->colCtx, spot.x, spot.z, &surfaceY, &waterBox) ||
                (fabsf(surfaceY - anchor->world.pos.y) > 40.0f)) {
                continue;
            }
            spot.y = anchor->world.pos.y;
        } else if (placement == EO_PLACE_AIR) {
            if (!Eo_FindFloor(play, &spot, 900.0f, &floorY)) {
                continue;
            }
            spot.y += lift;
        } else {
            if (!Eo_FindFloor(play, &spot, 120.0f, &floorY) || (fabsf(floorY - anchor->world.pos.y) > 60.0f)) {
                continue;
            }
            // No water on the spot: land enemies stay on land
            {
                f32 surfaceY;
                WaterBox* waterBox;

                if (WaterBox_GetSurface1(play, &play->colCtx, spot.x, spot.z, &surfaceY, &waterBox) &&
                    (surfaceY > floorY + 10.0f)) {
                    continue;
                }
            }
            spot.y = floorY;
        }

        *out = spot;
        return true;
    }
    return false;
}

static s32 sEoSpawningExtra = 0;

static Actor* Eo_SpawnExtra(PlayState* play, Actor* anchor, s16 actorId, Vec3f* pos, s16 rotY, s32 params) {
    Actor* actor;

    sEoSpawningExtra++;
    actor = Eo_SpawnAnyObject(play, actorId, pos->x, pos->y, pos->z, 0, rotY, 0, params);
    sEoSpawningExtra--;

    if (actor != NULL) {
        if (anchor != NULL) {
            actor->room = anchor->room;
        }
        actor->home.pos = *pos;
    }
    return actor;
}

static void EoCompanion_Roll(PlayState* play, Actor* actor, EoSceneInfo* scene) {
    s32 i;
    s32 spawnedAny = false;

    for (i = 0; i < ARRAY_COUNT(sEoCompanionRules); i++) {
        EoCompanionRule* rule = &sEoCompanionRules[i];
        s32 count;
        s32 n;

        if ((rule->triggerId != actor->id) || ((rule->region != EO_REGION_ANY) && (rule->region != scene->region))) {
            continue;
        }
        if (rule->nightOnly && !Eo_IsNight()) {
            continue;
        }
        if ((rule->params == EO_PARAMS_COPY) && !EoCompanion_CanCopy(actor)) {
            continue;
        }
        // Cross-kind companions only outside dungeons
        if ((rule->companionId != actor->id) && scene->isDungeon) {
            continue;
        }
        // One group per enemy
        if (spawnedAny || !Eo_Roll(rule->chance)) {
            continue;
        }

        count = rule->minCount + (s32)Rand_ZeroFloat(rule->maxCount - rule->minCount + 1);
        if (Eo_FrenzyLevel() == 2) {
            count++;
        }

        for (n = 0; n < count; n++) {
            Vec3f spot;

            if (EoCompanion_FindSpot(play, actor, rule->placement, rule->lift, &spot)) {
                s32 params = (rule->params == EO_PARAMS_COPY) ? actor->params : rule->params;

                if (Eo_SpawnExtra(play, actor, rule->companionId, &spot, actor->shape.rot.y, params) != NULL) {
                    spawnedAny = true;
                }
            }
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Tag-alongs for random-encounter spawners
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_TAG_QUEUE_MAX 6
#define EO_TAG_ALIVE_MAX 3

typedef enum EoTagMode {
    /* 0 */ EO_TAG_NEAR_TRIGGER, // appears in the air near the enemy that was spawned
    /* 1 */ EO_TAG_BEHIND_LINK,  // drops in behind Link
} EoTagMode;

typedef struct EoTagAlong {
    Actor* trigger;
    s16 delay;
    s16 actorId;
    s32 params;
    u8 mode;
} EoTagAlong;

static EoTagAlong sEoTagQueue[EO_TAG_QUEUE_MAX];
static Actor* sEoTagAlive[EO_TAG_ALIVE_MAX];

static s32 EoTag_AliveCount(void) {
    s32 i;
    s32 count = 0;

    for (i = 0; i < EO_TAG_ALIVE_MAX; i++) {
        if ((sEoTagAlive[i] != NULL) && (sEoTagAlive[i]->update != NULL)) {
            count++;
        }
    }
    return count;
}

static void EoTag_Queue(Actor* trigger, s16 delay, s16 actorId, s32 params, u8 mode) {
    s32 i;

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == NULL) {
            sEoTagQueue[i].trigger = trigger;
            sEoTagQueue[i].delay = delay;
            sEoTagQueue[i].actorId = actorId;
            sEoTagQueue[i].params = params;
            sEoTagQueue[i].mode = mode;
            return;
        }
    }
}

static void EoTag_Roll(PlayState* play, Actor* actor) {
    Actor* spawner = actor->parent;

    if ((spawner == NULL) || (EoTag_AliveCount() >= EO_TAG_ALIVE_MAX)) {
        return;
    }

    if ((spawner->id == ACTOR_EN_ENCOUNT1) && (actor->id == ACTOR_EN_GRASSHOPPER)) {
        // The swamp's Dragonflies sometimes hunt in pairs
        if (Eo_Roll(25)) {
            EoTag_Queue(actor, 30, ACTOR_EN_GRASSHOPPER, DRAGONFLY_TYPE_NORMAL, EO_TAG_NEAR_TRIGGER);
        }
    } else if ((spawner->id == ACTOR_EN_ENCOUNT3) && (actor->id == ACTOR_EN_JSO)) {
        // Garo ambushes: sometimes a second Garo drops in behind Link once the first one has had its say
        if (Eo_Roll(30)) {
            EoTag_Queue(actor, 90, ACTOR_EN_JSO, EN_JSO_PARAMS_SUMMONED, EO_TAG_BEHIND_LINK);
        }
    }
}

static void EoTag_Update(PlayState* play, Actor* trigger) {
    s32 i;

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        EoTagAlong* tag = &sEoTagQueue[i];
        Vec3f spot;
        s32 found;
        s32 slot;
        Actor* spawned;

        if (tag->trigger != trigger) {
            continue;
        }
        if (tag->delay > 0) {
            tag->delay--;
            continue;
        }
        // Wait for any text box (the Garo's line) and cutscene to end
        if (!Eo_CanAttack(play) || (Message_GetState(&play->msgCtx) != TEXT_STATE_NONE)) {
            continue;
        }

        tag->trigger = NULL;

        for (slot = 0; slot < EO_TAG_ALIVE_MAX; slot++) {
            if ((sEoTagAlive[slot] == NULL) || (sEoTagAlive[slot]->update == NULL)) {
                break;
            }
        }
        if (slot == EO_TAG_ALIVE_MAX) {
            continue;
        }

        if (tag->mode == EO_TAG_BEHIND_LINK) {
            found = Eo_PosBehindPlayer(play, 160.0f, &spot);
        } else {
            found = EoCompanion_FindSpot(play, trigger, EO_PLACE_AIR, 40, &spot);
        }
        if (!found) {
            continue;
        }

        spawned = Eo_SpawnExtra(play, NULL, tag->actorId, &spot,
                                Math_Vec3f_Yaw(&spot, &GET_PLAYER(play)->actor.world.pos), tag->params);
        if (spawned != NULL) {
            sEoTagAlive[slot] = spawned;
            Eo_Puff(play, &spot, 1.5f);
        }
    }
}

/* ---------------------------------------------------------------------------------------------------------------
 * Elites
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_ELITE_MAX 16
#define EO_ELITE_SCALE 1.25f

typedef struct EoElite {
    Actor* actor;
    s16 timer;
} EoElite;

static EoElite sEoElites[EO_ELITE_MAX];
static s32 sEoEliteCount = 0;

static s16 sEoEliteIds[] = {
    ACTOR_EN_FIREFLY, ACTOR_EN_SLIME, ACTOR_EN_TITE,      ACTOR_EN_SKB,    ACTOR_EN_GRASSHOPPER, ACTOR_EN_BB,
    ACTOR_EN_BBFALL,  ACTOR_EN_CROW,  ACTOR_EN_NEO_REEBA, ACTOR_EN_OKUTA, ACTOR_EN_RAT,         ACTOR_EN_SNOWMAN,
    ACTOR_EN_KAME,    ACTOR_EN_PP,    ACTOR_EN_WF,        ACTOR_EN_DINOFOS, ACTOR_EN_RR,        ACTOR_EN_DODONGO,
    ACTOR_EN_RD,      ACTOR_EN_DEKUBABA,
};

static s32 EoElite_Find(Actor* actor) {
    s32 i;

    if (sEoEliteCount == 0) {
        return -1;
    }
    for (i = 0; i < EO_ELITE_MAX; i++) {
        if (sEoElites[i].actor == actor) {
            return i;
        }
    }
    return -1;
}

s32 EoElite_Is(Actor* actor) {
    return EoElite_Find(actor) >= 0;
}

static void EoElite_Roll(PlayState* play, Actor* actor) {
    s32 i;
    s32 slot = -1;
    f32 chance = (Eo_FrenzyLevel() == 2) ? 10.0f : 5.0f;

    for (i = 0; i < ARRAY_COUNT(sEoEliteIds); i++) {
        if (sEoEliteIds[i] == actor->id) {
            break;
        }
    }
    if ((i == ARRAY_COUNT(sEoEliteIds)) || !EoCompanion_CanCopy(actor) || (actor->colChkInfo.health == 0) ||
        (Rand_ZeroFloat(100.0f) >= chance)) {
        return;
    }

    for (i = 0; i < EO_ELITE_MAX; i++) {
        if (sEoElites[i].actor == NULL) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        return;
    }

    sEoElites[slot].actor = actor;
    sEoElites[slot].timer = 0;
    sEoEliteCount++;
    actor->colChkInfo.health = CLAMP_MAX(actor->colChkInfo.health * 2, 250);
}

static void EoElite_Remove(s32 index) {
    sEoElites[index].actor = NULL;
    if (sEoEliteCount > 0) {
        sEoEliteCount--;
    }
}

static void EoElite_Sparkle(PlayState* play, EoElite* elite) {
    static Color_RGBA8 sPrim = { 255, 255, 180, 255 };
    static Color_RGBA8 sEnv = { 255, 190, 0, 0 };
    Actor* actor = elite->actor;
    Vec3f pos;
    Vec3f vel = { 0.0f, 0.6f, 0.0f };
    Vec3f accel = { 0.0f, 0.0f, 0.0f };

    elite->timer++;
    if ((elite->timer % 6) != 0) {
        return;
    }

    pos.x = actor->world.pos.x + Rand_CenteredFloat(40.0f);
    pos.y = actor->world.pos.y + 10.0f + Rand_ZeroFloat(40.0f);
    pos.z = actor->world.pos.z + Rand_CenteredFloat(40.0f);
    EffectSsKirakira_SpawnSmall(play, &pos, &vel, &accel, &sPrim, &sEnv);
}

/* ---------------------------------------------------------------------------------------------------------------
 * Hooks
 * --------------------------------------------------------------------------------------------------------------- */

#define EO_PENDING_MAX 96

static Actor* sEoPending[EO_PENDING_MAX];
static s32 sEoPendingCount = 0;

static s32 Eo_IsCandidate(s16 id) {
    s32 i;

    if ((id == ACTOR_EN_JSO) || (id == ACTOR_EN_GRASSHOPPER)) {
        return true; // tag-along triggers
    }
    for (i = 0; i < ARRAY_COUNT(sEoCompanionRules); i++) {
        if (sEoCompanionRules[i].triggerId == id) {
            return true;
        }
    }
    for (i = 0; i < ARRAY_COUNT(sEoEliteIds); i++) {
        if (sEoEliteIds[i] == id) {
            return true;
        }
    }
    return false;
}

void EnemyOverhaul_OnActorInit(Actor* actor) {
    s32 i;

    if ((gPlayState == NULL) || (gEoSpawningAnyObject > 0) || (sEoSpawningExtra > 0)) {
        return; // summons, companions and tag-alongs never bring anyone
    }
    if (!EO_ENCOUNTERS_ON() && !EO_ELITES_ON()) {
        return;
    }
    if ((actor->update == NULL) || !Eo_IsCandidate(actor->id)) {
        return;
    }

    // Decided on its first update, once the rest of the room (doors, chests) is loaded
    for (i = 0; i < EO_PENDING_MAX; i++) {
        if (sEoPending[i] == NULL) {
            sEoPending[i] = actor;
            sEoPendingCount++;
            return;
        }
    }
}

static void Eo_ProcessNewEnemy(PlayState* play, Actor* actor) {
    EoSceneInfo* scene = EoScene_Get(play);

    if ((scene == NULL) || (actor->update == NULL)) {
        return;
    }

    if (EO_ELITES_ON()) {
        EoElite_Roll(play, actor);
    }

    if (!EO_ENCOUNTERS_ON() || (scene->isDungeon && !EO_ENCOUNTERS_DUNGEONS_ON())) {
        return;
    }

    if (actor->parent != NULL) {
        // Spawned by something else: a random-encounter spawner's enemy can get a tag-along
        EoTag_Roll(play, actor);
        return;
    }

    if (EoRoom_HasClearReward(play, actor->room)) {
        return;
    }

    EoCompanion_Roll(play, actor, scene);
}

void EnemyOverhaul_OnActorUpdate(Actor* actor) {
    PlayState* play = gPlayState;
    s32 i;

    if (play == NULL) {
        return;
    }

    if (sEoPendingCount > 0) {
        for (i = 0; i < EO_PENDING_MAX; i++) {
            if (sEoPending[i] == actor) {
                sEoPending[i] = NULL;
                sEoPendingCount--;
                Eo_ProcessNewEnemy(play, actor);
                break;
            }
        }
    }

    if (sEoEliteCount > 0) {
        i = EoElite_Find(actor);
        if (i >= 0) {
            if (!EO_ELITES_ON()) {
                EoElite_Remove(i);
            } else {
                EoElite_Sparkle(play, &sEoElites[i]);
            }
        }
    }

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == actor) {
            EoTag_Update(play, actor);
            break;
        }
    }
}

void EnemyOverhaul_OnActorPreDraw(Actor* actor) {
    if ((sEoEliteCount > 0) && (EoElite_Find(actor) >= 0)) {
        Matrix_Scale(EO_ELITE_SCALE, EO_ELITE_SCALE, EO_ELITE_SCALE, MTXMODE_APPLY);
    }
}

void EnemyOverhaul_OnActorKill(Actor* actor) {
    // ITEM00_FLEXIBLE picks whatever Link is shortest of (hearts, magic, arrows, bombs...)
    static u8 sEliteDrops[] = { ITEM00_RUPEE_RED, ITEM00_RUPEE_PURPLE, ITEM00_3_HEARTS, ITEM00_FLEXIBLE };
    s32 i = EoElite_Find(actor);

    if (i < 0) {
        return;
    }

    if ((gPlayState != NULL) && (actor->colChkInfo.health == 0)) {
        Item_DropCollectible(gPlayState, &actor->world.pos,
                             sEliteDrops[(s32)Rand_ZeroFloat(ARRAY_COUNT(sEliteDrops)) % ARRAY_COUNT(sEliteDrops)]);
    }
    EoElite_Remove(i);
}

void EoEncounters_OnActorDestroy(Actor* actor) {
    s32 i;

    if (sEoPendingCount > 0) {
        for (i = 0; i < EO_PENDING_MAX; i++) {
            if (sEoPending[i] == actor) {
                sEoPending[i] = NULL;
                sEoPendingCount--;
            }
        }
    }

    i = EoElite_Find(actor);
    if (i >= 0) {
        EoElite_Remove(i);
    }

    for (i = 0; i < EO_TAG_QUEUE_MAX; i++) {
        if (sEoTagQueue[i].trigger == actor) {
            sEoTagQueue[i].trigger = NULL;
        }
    }
    for (i = 0; i < EO_TAG_ALIVE_MAX; i++) {
        if (sEoTagAlive[i] == actor) {
            sEoTagAlive[i] = NULL;
        }
    }
}
