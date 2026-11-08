#include "StoryProgress.h"

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
}

namespace StoryProgress {

// ---- Save-file checks -------------------------------------------------------------------------
// Everything used to complete a milestone survives the Song of Time: quest items (Remains, songs), inventory
// items and masks, and the few event flags the game keeps between cycles (areas entered, Bombers' Notebook).

static bool Has(int32_t item) {
    return INV_CONTENT(item) == item;
}

static bool Quest(int32_t questItem) {
    return CHECK_QUEST_ITEM(questItem) != 0;
}

static bool Week(int32_t flag) {
    return CHECK_WEEKEVENTREG(flag) != 0;
}

static bool HasMagic() {
    return gSaveContext.save.saveInfo.playerData.isMagicAcquired != 0;
}

static bool HasMirrorShield() {
    return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) >= EQUIP_VALUE_SHIELD_MIRROR;
}

static constexpr bool kCycle = true; // the Song of Time undoes this step
static constexpr bool kKeep = false; // this step stays done

// ---- The story --------------------------------------------------------------------------------

static std::vector<Milestone> BuildMainMilestones() {
    std::vector<Milestone> m;

    m.push_back(
        { "The First Three Days",
          "A stolen ocarina and a cursed form",
          "Clock Town",
          {
              { "You start as a Deku Scrub with no ocarina, and the moon falls in three days. Head out of the "
                "Clock Tower into South Clock Town, then take the alley at the bottom-left into the Laundry Pool "
                "and catch the Stray Fairy floating there.",
                [] { return HasMagic(); }, kCycle },
              { "Take the Stray Fairy to the Great Fairy Fountain in North Clock Town (the cave behind the "
                "slide). She gives you magic, which lets a Deku Scrub shoot bubbles (hold B).",
                [] { return HasMagic(); }, kKeep },
              { "In North Clock Town, pop the balloon above the tree with a bubble. The Bombers then play "
                "hide-and-seek: find all five kids around town (North, East and West Clock Town) and they tell "
                "you their secret code.",
                [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_LEARNED_SECRET_CODE); }, kKeep },
              { "In East Clock Town, tell the code to the Bomber guarding the passage behind the stairs. Go "
                "through the tunnel to the Astral Observatory, climb up and look through the telescope at the "
                "Clock Tower until something falls out of the moon. Pick up the Moon's Tear in the field just "
                "outside.",
                nullptr, kCycle },
              { "Back in South Clock Town, trade the Moon's Tear to the Business Scrub at the flower for the "
                "Land Title Deed. His flower is now yours.",
                nullptr, kCycle },
              { "Wait until midnight on the Final Day (the countdown on screen). The Clock Tower door opens. "
                "Launch from the flower onto the platform and climb to the top. Shoot the Skull Kid with a "
                "bubble so he drops the Ocarina of Time.",
                [] { return Has(ITEM_OCARINA_OF_TIME); }, kKeep },
              { "A memory of Princess Zelda teaches the Song of Time. Play it to return to the Dawn of the "
                "First Day. Before playing it, deposit your Rupees in the bank in West Clock Town: Rupees in "
                "your wallet are lost.",
                [] { return Quest(QUEST_SONG_TIME); }, kKeep },
              { "Go back inside the Clock Tower. The Happy Mask Salesman teaches the Song of Healing, which turns "
                "you back into a human and leaves the Deku Mask.",
                [] { return Has(ITEM_MASK_DEKU) && Quest(QUEST_SONG_HEALING); }, kKeep },
          },
          [] { return Has(ITEM_MASK_DEKU) && Quest(QUEST_SONG_TIME); } });

    m.push_back(
        { "Woodfall Temple",
          "Poisoned waters to the south",
          "Southern Swamp, Deku Palace and Woodfall",
          {
              { "Go south from Termina Field into the Southern Swamp and hit the Owl Statue with your sword. "
                "Take the left path into the Woods of Mystery and follow the monkey to the injured Koume.",
                nullptr, kCycle },
              { "Climb into the Magic Hags' Potion Shop (the big cauldron) and talk to Kotake: she gives you a "
                "bottle of Red Potion. Don't drink it; give it to Koume in the woods.",
                [] { return Week(WEEKEVENTREG_RECEIVED_KOTAKE_BOTTLE); }, kKeep },
              { "Koume, now at the Swamp Tourist Center, gives you the Pictograph Box and a free boat ride to "
                "the Deku Palace.",
                [] { return Has(ITEM_PICTOGRAPH_BOX); }, kKeep },
              { "Wear the Deku Mask to get past the guards and hear the King accuse the monkey. Then sneak "
                "through the east (right-hand) garden, out of the guards' sight (easier at night, when their "
                "view shows as dots). At the end, drop into the hole: the Bean Seller is inside.",
                [] { return Week(WEEKEVENTREG_ENTERED_DEKU_PALACE); }, kCycle },
              { "Buy a Magic Bean and fill a bottle with Spring Water in the corner of the grotto. As a Deku "
                "Scrub, follow the lily pads east to the soft soil, plant the bean and water it.",
                nullptr, kCycle },
              { "Ride the bean plant up, then fly from Deku Flower to Deku Flower to the top of the gardens and "
                "into the passage to the monkey's cage. Play the ocarina as a Deku Scrub to learn the Sonata of "
                "Awakening.",
                [] { return Quest(QUEST_SONG_SONATA); }, kKeep },
              { "From the Deku Palace entrance, go east again, take the passage on your right and launch from "
                "the flowers up to the owl. Read the tablet to learn the Song of Soaring (it warps you to any "
                "Owl Statue you've hit).",
                [] { return Quest(QUEST_SONG_SOARING); }, kKeep },
              { "Fly across to Woodfall and hit its Owl Statue. Stand on the Deku symbol on the platform and "
                "play the Sonata of Awakening as a Deku Scrub: Woodfall Temple rises.",
                [] { return Week(WEEKEVENTREG_ENTERED_WOODFALL_TEMPLE); }, kCycle },
              { "Inside, beat the Dinolfos for the Hero's Bow. Use it to hit the eye switches and to shoot "
                "arrows through the torches to light them.",
                [] { return Has(ITEM_BOW); }, kKeep },
              { "Beat Gekko and the Snapper for the Boss Key, then make your way up from the main room to the "
                "boss door.",
                nullptr, kCycle },
              { "Odolwa: stun him with arrows or Deku Nuts, then slash. Keep moving when he dances and swings. "
                "Take Odolwa's Remains; the Giants teach you the Oath to Order.",
                [] { return Quest(QUEST_REMAINS_ODOLWA); }, kKeep },
              { "Catch the Deku Princess in an empty bottle in the room past Odolwa and take her to the King "
                "in the Deku Palace. The swamp is now clean, and the Deku Butler's race (Mask of Scents) opens.",
                nullptr, kCycle },
          },
          [] { return Quest(QUEST_REMAINS_ODOLWA); } });

    m.push_back(
        { "Snowhead Temple",
          "An endless winter in the north",
          "Mountain Village, Goron Village and Snowhead",
          {
              { "Go north from Termina Field. Shoot the icicles above the ice blocking the path with two arrows, "
                "then go on to Mountain Village and hit its Owl Statue.",
                nullptr, kKeep },
              { "In Goron Village, follow the owl's falling feathers across the invisible platforms over the "
                "lake to the cave with the Lens of Truth.",
                [] { return Has(ITEM_LENS_OF_TRUTH); }, kKeep },
              { "Use the Lens in Mountain Village to see Darmani's ghost. Follow him into the graveyard and play "
                "the Song of Healing: you get the Goron Mask.",
                [] { return Has(ITEM_MASK_GORON); }, kKeep },
              { "Pull back Darmani's grave and fill a bottle with Hot Spring Water.", nullptr, kCycle },
              { "The Goron Elder is frozen in a big snowball on the way to Snowhead (by the Owl Statue on the "
                "Final Day). Smash it as a Goron (punch it) and pour Hot Spring Water on him. He teaches you the "
                "Lullaby Intro.",
                [] { return Quest(QUEST_SONG_LULLABY_INTRO) || Quest(QUEST_SONG_LULLABY); }, kKeep },
              { "In Goron Village, pound the ground next to the shrine door as a Goron to open it. Play the "
                "Lullaby Intro to the Elder's crying son: he finishes it and you learn the Goron Lullaby.",
                [] { return Quest(QUEST_SONG_LULLABY); }, kKeep },
              { "On the way to Snowhead, the giant Goron blows you back. Play the Goron Lullaby as a Goron to "
                "put him to sleep, then roll over the narrow ledges to Snowhead Temple.",
                [] { return Week(WEEKEVENTREG_ENTERED_SNOWHEAD_TEMPLE); }, kCycle },
              { "In the temple, beat the Wizzrobe on the second floor for the Fire Arrows. Use them to melt ice, "
                "and pound the central pillar down a level at a time as a Goron.",
                [] { return Has(ITEM_ARROW_FIRE); }, kKeep },
              { "Beat the second Wizzrobe for the Boss Key, flatten the pillar fully and roll up to the boss door.",
                nullptr, kCycle },
              { "Goht: shoot Fire Arrows to melt the ice, then roll after him as a Goron (with magic for spikes) "
                "and ram him. Take Goht's Remains. Spring comes to the mountain.",
                [] { return Quest(QUEST_REMAINS_GOHT); }, kKeep },
          },
          [] { return Quest(QUEST_REMAINS_GOHT); } });

    m.push_back(
        { "Romani Ranch and Epona",
          "A boulder on the road to the farm",
          "Milk Road and Romani Ranch",
          {
              { "Powder Kegs are only sold to Gorons who prove themselves. As a Goron, talk to the Powder Keg "
                "Goron in Goron Village and pass his test: carry a keg to the boulder he points you to (it blocks "
                "the Goron Racetrack) and blow it up. After that, Gorons can buy kegs from him or from the Bomb "
                "Shop in West Clock Town.",
                [] { return Has(ITEM_POWDER_KEG); }, kCycle },
              { "On the First Day, go to Milk Road (south-west of Termina Field), hit the Owl Statue, and blow up "
                "the huge boulder blocking the ranch with a Powder Keg.",
                [] { return Week(WEEKEVENTREG_ENTERED_ROMANI_RANCH); }, kCycle },
              { "On the First Day (6 AM to 6 PM), talk to Romani in the field and pass her balloon practice with "
                "the bow. She teaches you Epona's Song, and Epona is yours.",
                [] { return Quest(QUEST_SONG_EPONA); }, kKeep },
              { "Optional but worth it: return at 2:30 AM on the first night to defend the barn from the "
                "aliens until dawn (bottle of milk). Then on the Second Day at 6 PM, escort Cremia's milk cart "
                "to Clock Town for Romani's Mask.",
                [] { return Has(ITEM_MASK_ROMANI); }, kCycle },
          },
          [] { return Quest(QUEST_SONG_EPONA); } });

    m.push_back(
        { "Great Bay Temple",
          "Murky waters to the west",
          "Great Bay Coast, Pirates' Fortress and Zora Cape",
          {
              { "Call Epona in Termina Field (Epona's Song) and jump the fence to the west to reach Great Bay "
                "Coast. Hit the Owl Statue.",
                [] { return Week(WEEKEVENTREG_ENTERED_GREAT_BAY_COAST); }, kKeep },
              { "Swim out to the injured Zora, push him to shore, and play the Song of Healing: you get the "
                "Zora Mask.",
                [] { return Has(ITEM_MASK_ZORA); }, kKeep },
              { "Swim along the coast to the Pirates' Fortress. In the sewers, pound the switch as a Goron and "
                "swim through as a Zora; inside, stay out of the guards' sight. From the room overlooking the leader, shoot the "
                "beehive through the window: the bees chase the pirates out. Take the Hookshot from the chest.",
                [] { return Has(ITEM_HOOKSHOT); }, kKeep },
              { "Collect the four Zora Eggs in the fortress (next to the Hookshot chest, the south side with a "
                "Hookshot target, and the two rooms behind pirate fights). Carry them in bottles. Take a picture "
                "of a pirate before you leave.",
                nullptr, kCycle },
              { "At the Marine Research Lab (the house on stilts), pour the eggs into the big tank. Give the "
                "pirate picture to the Fisherman in the hut on the right for a Seahorse in a bottle.",
                nullptr, kCycle },
              { "Swim between the two big rocks to Pinnacle Rock and release the Seahorse to guide you. Beat the "
                "sea snakes for the last three eggs, and pour them into the lab's tank: the baby Zoras teach you "
                "the New Wave Bossa Nova.",
                [] { return Quest(QUEST_SONG_BOSSA_NOVA); }, kKeep },
              { "Go to Zora Cape, hit its Owl Statue, and play the New Wave Bossa Nova as a Zora next to Lulu. A "
                "giant turtle rises: Hookshot onto it to reach Great Bay Temple.",
                nullptr, kCycle },
              { "In the temple, beat Wart for the Ice Arrows. Freeze water to make platforms and stop the pipes. "
                "Turn the red and green water flows on and off to reach new rooms.",
                [] { return Has(ITEM_ARROW_ICE); }, kKeep },
              { "Beat Gekko and the Mad Jelly for the Boss Key.", nullptr, kCycle },
              { "Gyorg: shoot him with arrows (or hit him with the Zora's electric barrier) when he comes up, "
                "then swim in and attack while he's stunned. Take Gyorg's Remains.",
                [] { return Quest(QUEST_REMAINS_GYORG); }, kKeep },
          },
          [] { return Quest(QUEST_REMAINS_GYORG); } });

    m.push_back(
        { "Ikana",
          "A valley haunted by the dead",
          "Ikana Graveyard, Ikana Canyon and Ikana Castle",
          {
              { "On Milk Road, race the Gorman Brothers on Epona (Day 1 or 2, 10 Rupees) and win the Garo's "
                "Mask.",
                [] { return Has(ITEM_MASK_GARO); }, kKeep },
              { "East of Termina Field, jump the fences on Epona. At the big cliff, wear the Garo's Mask: a "
                "tree appears. Hookshot up into Ikana Canyon.",
                [] { return Week(WEEKEVENTREG_ENTERED_IKANA_CANYON); }, kKeep },
              { "In Ikana Graveyard, play the Sonata of Awakening next to the huge skull at the far right. Chase "
                "Captain Keeta and beat him for the Captain's Hat.",
                [] { return Has(ITEM_MASK_CAPTAIN); }, kKeep },
              { "On the first night, wear the Captain's Hat in the graveyard and tell the Stalchildren to open "
                "the grave. Inside, light three torches with Fire Arrows, beat the Iron Knuckle, and read Flat's "
                "tombstone: you learn the Song of Storms.",
                [] { return Quest(QUEST_SONG_STORMS); }, kKeep },
              { "In Ikana Canyon, freeze the Octoroks with Ice Arrows to cross the river and Hookshot up to the "
                "Owl Statue. Follow the dry river to the cave and play the Song of Storms to drive out Sharp.",
                nullptr, kCycle },
              { "With water flowing, sneak into the Music Box House (the Stone Mask helps), go downstairs and "
                "play the Song of Healing to Pamela's father: you get the Gibdo Mask.",
                [] { return Has(ITEM_MASK_GIBDO); }, kKeep },
              { "Beneath the Well (west side of the canyon), wear the Gibdo Mask and give each Gibdo what it "
                "asks for (Magic Beans, Deku Nuts, Bombs, a Fish, a Big Poe, Milk...). Light the torches in the "
                "last room for the Mirror Shield.",
                [] { return HasMirrorShield(); }, kKeep },
              { "Ancient Castle of Ikana: reflect light with the Mirror Shield onto the sun blocks, and drop a "
                "Powder Keg through the cracked roof to open the throne room.",
                [] { return Week(WEEKEVENTREG_ENTERED_IKANA_CASTLE); }, kCycle },
              { "Igos du Ikana: burn the curtains with Fire Arrows, beat his two lackeys and Igos, and shine "
                "light on their bodies. Igos teaches you the Elegy of Emptiness.",
                [] { return Quest(QUEST_SONG_ELEGY); }, kKeep },
              { "Climb Stone Tower: at each switch play the Elegy of Emptiness to leave a statue of your current "
                "form (Human, Goron and Zora) on the switches, then Hookshot up. At the top, hit the Owl Statue "
                "and enter Stone Tower Temple.",
                [] { return Week(WEEKEVENTREG_ENTERED_STONE_TOWER_TEMPLE); }, kKeep },
          },
          [] { return Quest(QUEST_SONG_ELEGY) && Week(WEEKEVENTREG_ENTERED_STONE_TOWER_TEMPLE); } });

    m.push_back(
        { "Stone Tower Temple",
          "A temple with no up or down",
          "Stone Tower Temple",
          {
              { "Use the Mirror Shield, Elegy statues and the Goron and Zora forms to get through the right-side-up "
                "temple. Beat the Garo Master for the Light Arrows.",
                [] { return Has(ITEM_ARROW_LIGHT); }, kKeep },
              { "Outside, shoot the red emblem with a Light Arrow: the whole temple flips upside down.",
                [] { return Week(WEEKEVENTREG_ENTERED_STONE_TOWER_TEMPLE_INVERTED); }, kKeep },
              { "In the inverted temple, beat Gomess for the Boss Key (Light Arrows break his bat shield).",
                nullptr, kCycle },
              { "Beat the Eyegore for the Giant's Mask, then go to the boss.",
                [] { return Has(ITEM_MASK_GIANT); }, kKeep },
              { "Twinmold: wear the Giant's Mask (it uses magic) and punch the worms' heads and tails. Take "
                "Twinmold's Remains.",
                [] { return Quest(QUEST_REMAINS_TWINMOLD); }, kKeep },
          },
          [] { return Quest(QUEST_REMAINS_TWINMOLD); } });

    m.push_back(
        { "The Moon",
          "Face the falling moon",
          "Clock Tower rooftop and the Moon",
          {
              { "With all four Remains, be on top of the Clock Tower at midnight on the Final Day and play the "
                "Oath to Order. The Giants catch the moon.",
                [] { return Week(WEEKEVENTREG_ENTERED_THE_MOON); }, kKeep },
              { "On the Moon, the masked children send you through trials (Deku, Goron, Zora and Link). Give "
                "them all your masks: a child with every other mask gives you the Fierce Deity's Mask.",
                [] { return Has(ITEM_MASK_FIERCE_DEITY); }, kKeep },
              { "Talk to the child under the tree and face Majora. Fierce Deity's Mask makes the fight much "
                "easier.",
                nullptr, kKeep },
          },
          [] { return false; } });

    return m;
}

static std::vector<SideThread> BuildSideThreads() {
    std::vector<SideThread> s;

    // ---- The 24 masks: what each is for and where it comes from ----

    s.push_back({ "The 24 Masks",
                  "Where every mask comes from. Masks are kept through the Song of Time. All 20 non-transforming "
                  "masks are needed for the Fierce Deity's Mask.",
                  {
                      { "Deku Mask: the Happy Mask Salesman, after the first three days.",
                        [] { return Has(ITEM_MASK_DEKU); }, kKeep },
                      { "Goron Mask: Song of Healing to Darmani's ghost in Mountain Village (seen with the Lens).",
                        [] { return Has(ITEM_MASK_GORON); }, kKeep },
                      { "Zora Mask: push the injured Zora on Great Bay Coast to shore, then Song of Healing.",
                        [] { return Has(ITEM_MASK_ZORA); }, kKeep },
                      { "Great Fairy's Mask: return the Clock Town Stray Fairy (Laundry Pool) to the fountain in "
                        "North Clock Town as a human.",
                        [] { return Has(ITEM_MASK_GREAT_FAIRY); }, kKeep },
                      { "Bremen Mask: Guru-Guru, the musician in the Laundry Pool, at night on Day 1 or 2.",
                        [] { return Has(ITEM_MASK_BREMEN); }, kKeep },
                      { "Blast Mask: midnight on the first night in North Clock Town, stop Sakon from robbing the "
                        "old lady (hit him or the bag).",
                        [] { return Has(ITEM_MASK_BLAST); }, kKeep },
                      { "Kafei's Mask: Madame Aroma in the Mayor's Residence (East Clock Town), Day 1 from 10 AM.",
                        [] { return Has(ITEM_MASK_KAFEIS_MASK); }, kKeep },
                      { "Bunny Hood: Grog in the Cucco Shack at Romani Ranch. Wear the Bremen Mask and march the "
                        "chicks until all of them grow up.",
                        [] { return Has(ITEM_MASK_BUNNY); }, kKeep },
                      { "Mask of Truth: clear the Swamp Spider House (30 Gold Skulltulas) in the Southern Swamp.",
                        [] { return Has(ITEM_MASK_TRUTH); }, kKeep },
                      { "Keaton Mask: Final Day after 1 PM, the Curiosity Shop owner in Kafei's hideout in the "
                        "Laundry Pool (part of Anju & Kafei).",
                        [] { return Has(ITEM_MASK_KEATON); }, kKeep },
                      { "Postman's Hat: Final Day after 6 PM, give the Special Delivery to Mama to the postman at "
                        "the Post Office (West Clock Town), follow him and talk to him when he comes out.",
                        [] { return Has(ITEM_MASK_POSTMAN); }, kKeep },
                      { "All-Night Mask: after saving the old lady from Sakon, buy it at the Curiosity Shop "
                        "(West Clock Town) on the final night for 500 Rupees (Giant's Wallet needed).",
                        [] { return Has(ITEM_MASK_ALL_NIGHT); }, kKeep },
                      { "Romani's Mask: defend the ranch from the aliens (first night, 2:30 AM), then escort "
                        "Cremia's cart on Day 2 at 6 PM.",
                        [] { return Has(ITEM_MASK_ROMANI); }, kKeep },
                      { "Circus Leader's Mask: wear Romani's Mask to enter the Milk Bar on night 1 or 2, and play "
                        "for Toto's band as Deku, Goron, Zora and Human (the Ballad of the Windfish).",
                        [] { return Has(ITEM_MASK_CIRCUS_LEADER); }, kKeep },
                      { "Couple's Mask: finish Anju & Kafei (Final Day, before dawn, in Anju's room at the Inn).",
                        [] { return Has(ITEM_MASK_COUPLE); }, kKeep },
                      { "Kamaro's Mask: at night, play the Song of Healing to the dancing ghost on the big "
                        "mushroom rock in north Termina Field.",
                        [] { return Has(ITEM_MASK_KAMARO); }, kKeep },
                      { "Stone Mask: in Ikana (on the road before the canyon), use the Lens to see Shiro, the "
                        "invisible soldier, and give him a Red Potion.",
                        [] { return Has(ITEM_MASK_STONE); }, kKeep },
                      { "Mask of Scents: after saving the Deku Princess, race the Deku Butler in the Deku Shrine "
                        "(Deku Palace) and keep up with him.",
                        [] { return Has(ITEM_MASK_SCENTS); }, kKeep },
                      { "Don Gero's Mask: give a Rock Sirloin to the hungry Goron on a high ledge in Mountain "
                        "Village (in winter). Make the chandelier in the Goron Shrine spin (light its torches) "
                        "and roll into it as a Goron to drop the Rock Sirloin.",
                        [] { return Has(ITEM_MASK_DON_GERO); }, kKeep },
                      { "Captain's Hat: wake Captain Keeta with the Sonata at the big skull in Ikana Graveyard "
                        "and win the race.",
                        [] { return Has(ITEM_MASK_CAPTAIN); }, kKeep },
                      { "Garo's Mask: win the horse race against the Gorman Brothers on Milk Road.",
                        [] { return Has(ITEM_MASK_GARO); }, kKeep },
                      { "Gibdo Mask: heal Pamela's father in the Music Box House basement, Ikana Canyon.",
                        [] { return Has(ITEM_MASK_GIBDO); }, kKeep },
                      { "Giant's Mask: the Eyegore in Stone Tower Temple (inverted).",
                        [] { return Has(ITEM_MASK_GIANT); }, kKeep },
                      { "Fierce Deity's Mask: give the masked children on the Moon all 20 other masks.",
                        [] { return Has(ITEM_MASK_FIERCE_DEITY); }, kKeep },
                  },
                  [] {
                      for (int32_t item = ITEM_MASK_DEKU; item <= ITEM_MASK_GIANT; item++) {
                          if (!Has(item)) {
                              return false;
                          }
                      }
                      return true;
                  } });

    // ---- Anju & Kafei (all in one cycle) ----
    // The ticks come from the Bombers' Notebook, which remembers each event across cycles; the quest itself
    // has to be done from start to finish in a single cycle.

    s.push_back({ "Anju & Kafei",
                  "The whole quest must be done in a single three-day cycle. Ticks show which events you've "
                  "already seen (from the Bombers' Notebook). Rewards: Keaton Mask, Postman's Hat, Couple's "
                  "Mask.",
                  {
                      { "Day 1, 10 AM - 12 PM: in the Mayor's Residence (East Clock Town), talk to Madame Aroma in "
                        "the room right of the receptionist for Kafei's Mask.",
                        [] { return Has(ITEM_MASK_KAFEIS_MASK); }, kCycle },
                      { "Day 1, 2:10 PM - 8 PM: wear Kafei's Mask and talk to Anju at the Stock Pot Inn desk. She "
                        "asks you to meet her late tonight. (Talk to her without the mask between 2:10 and 3:50 "
                        "PM first if you want the Room Key.)",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_PROMISED_MIDNIGHT_MEETING); }, kCycle },
                      { "Night 1, midnight: be in the Inn's kitchen (front door with the key, or the roof as a "
                        "Deku Scrub). Anju gives you the Letter to Kafei.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_RECEIVED_LETTER_TO_KAFEI); }, kCycle },
                      { "Day 2, before 9 AM: post the letter in any red postbox.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_DEPOSITED_LETTER_TO_KAFEI); }, kCycle },
                      { "Day 2, 3 PM: wait in the Laundry Pool. The postman rings the bell; go through the door "
                        "the masked kid opens and wait inside. At 4:10 PM, talk to Kafei: he gives you the "
                        "Pendant of Memories.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_RECEIVED_PENDANT_OF_MEMORIES); }, kCycle },
                      { "Day 2 or 3, 10 AM - 7 PM: give the Pendant to Anju at the Inn. She stays in town to wait.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_DELIVERED_PENDANT_OF_MEMORIES); }, kCycle },
                      { "Final Day, 1 PM - 6 PM: go back to Kafei's hideout in the Laundry Pool. The Curiosity Shop "
                        "owner gives you the Keaton Mask and the Special Delivery to Mama.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_RECEIVED_KEATON_MASK); }, kCycle },
                      { "Final Day, 6 PM: in Ikana Canyon, wait out of sight near Sakon's Hideout (the Stone Mask "
                        "helps). Follow Kafei in, and take turns with him through the rooms until he gets the "
                        "Sun's Mask back.",
                        [] { return Week(WEEKEVENTREG_BOMBERS_NOTEBOOK_EVENT_ESCAPED_SAKONS_HIDEOUT); }, kCycle },
                      { "Optional: give the Special Delivery to Mama to the postman at the Post Office (Postman's "
                        "Hat), or to Madame Aroma in the Milk Bar (Chateau Romani).",
                        [] { return Has(ITEM_MASK_POSTMAN); }, kCycle },
                      { "Final night: go to Anju's room at the top of the Inn's stairs and wait until about 4:30 "
                        "AM (1:30:00 left on the clock). Kafei arrives, and they give you the Couple's Mask.",
                        [] { return Has(ITEM_MASK_COUPLE); }, kKeep },
                  },
                  [] { return Has(ITEM_MASK_COUPLE); } });

    // ---- Great Fairies ----

    s.push_back({ "Great Fairies",
                  "Each temple hides 15 Stray Fairies. Gather all 15 in one cycle (the count resets with the Song "
                  "of Time) and bring them to that region's Great Fairy Fountain. The rewards are kept.",
                  {
                      { "Clock Town (1 fairy, in the Laundry Pool): magic as a Deku Scrub, then the Great Fairy's "
                        "Mask as a human.",
                        [] { return HasMagic() && Has(ITEM_MASK_GREAT_FAIRY); }, kKeep },
                      { "Woodfall Temple: the Great Spin Attack (hold B longer). The fountain is next to Woodfall "
                        "Temple.",
                        [] { return Week(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK); }, kKeep },
                      { "Snowhead Temple: double magic. The fountain is on the path to Snowhead.",
                        [] { return gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired != 0; }, kKeep },
                      { "Great Bay Temple: double defense. The fountain is at Zora Cape.",
                        [] { return gSaveContext.save.saveInfo.playerData.doubleDefense != 0; }, kKeep },
                      { "Stone Tower Temple: the Great Fairy's Sword. The fountain is in Ikana Canyon.",
                        [] { return Has(ITEM_SWORD_GREAT_FAIRY); }, kKeep },
                  },
                  [] {
                      return Has(ITEM_MASK_GREAT_FAIRY) && Week(WEEKEVENTREG_RECEIVED_GREAT_SPIN_ATTACK) &&
                             gSaveContext.save.saveInfo.playerData.isDoubleMagicAcquired &&
                             gSaveContext.save.saveInfo.playerData.doubleDefense && Has(ITEM_SWORD_GREAT_FAIRY);
                  } });

    // ---- Spider Houses ----

    s.push_back({ "Spider Houses",
                  "Each Spider House has 30 Gold Skulltulas, all to be collected in one cycle.",
                  {
                      { "Swamp Spider House (the big tree house in the Southern Swamp, near the path to the Deku "
                        "Palace): break the pots, burn the webs, and use the bean sprouts and Deku Flowers. Reward: the Mask of "
                        "Truth.",
                        [] { return Has(ITEM_MASK_TRUTH); }, kKeep },
                      { "Ocean Spider House (the house on Great Bay Coast): break open the boarded entrance and "
                        "use the Hookshot and Captain's Hat. Clear it on Day 1 for "
                        "the Giant's Wallet.",
                        [] { return CUR_UPG_VALUE(UPG_WALLET) >= 2; }, kKeep },
                  },
                  [] { return Has(ITEM_MASK_TRUTH) && (CUR_UPG_VALUE(UPG_WALLET) >= 2); } });

    return s;
}

const std::vector<Milestone>& MainMilestones() {
    static const std::vector<Milestone> milestones = BuildMainMilestones();
    return milestones;
}

const std::vector<SideThread>& SideThreads() {
    static const std::vector<SideThread> threads = BuildSideThreads();
    return threads;
}

int32_t CurrentMilestoneIndex() {
    const auto& milestones = MainMilestones();
    for (size_t i = 0; i < milestones.size(); i++) {
        if (!milestones[i].done()) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

} // namespace StoryProgress
