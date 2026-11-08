# 2S2H-MM-Overhaul

A modified version of [2 Ship 2 Harkinian](https://github.com/HarbourMasters/2ship2harkinian), the PC port of *The Legend of Zelda: Majora's Mask*, based on [lightmanLP's mouse-support build M3.1.0](https://github.com/lightmanLP/2ship2harkinian/releases/tag/M3.1.0). It adds tougher and smarter bosses, minibosses and enemies, sword projectiles, and better Epona controls.

Every change can be switched on or off in the in-game menu (press **F1** for the menu bar). They're **on by default**; turning one off gives you the original behavior back.

This is a fan project and is not affiliated with HarbourMasters. You still need your own legally acquired Majora's Mask ROM.

---

## What's changed

### Boss Overhaul
*Enhancements → Difficulty Options* - switches: **Boss HP**, **Extra Attacks**, **Majora Resists Fierce Deity**

| Boss | Health | What's new |
| - | - | - |
| **Odolwa** | 2× | **Fire wave:** slashing from range sends 3 flames racing along the floor at you (5 when enraged); walls stop them. **Flame ring:** his spin attack throws off a ring of embers. Up to 8 bugs at a time once enraged, instead of 5. |
| **Goht** | 2× | **Electric mines:** while running it leaves floating balls of lightning on the track behind it (two side by side once enraged). At half health it fires its homing electric ball on its own (vanilla only did after a Goron-spike hit), and starts throwing bombs at half health instead of a third. |
| **Gyorg** | 2× | **Water bullets:** while you're on the central platform, it bursts volleys of 3 out of the lake (5 when enraged). They lob onto the platform and can knock you back, often straight into the water. |
| **Twinmold** | 2× | Fully reworked - see below. |
| **Majora** | 3× in every phase (Mask 14→42, Incarnation 30→90, Wrath 40→120) | See below. |

#### Twinmold
- **Dragon breath:** the dragon slows, draws fire or frost into its glowing jaws (lighting up the arena), then pours out a stream of large fireballs or frost balls that sweeps after you with a lag. Fire burns, frost freezes, and neither can be blocked with a shield.
- **Cover:** ruins stop the breath and are no longer worn down by it, so hiding behind one works.
- **Meteor storm (red) and hailstorm (blue):** while flying high, the dragon spits balls into the sky that come down as meteors or giant icicles around you, the first where you were standing. Each spot is marked by a ring that closes in about 1.5 seconds before impact, and the blast can smash ruins.
- **Eruption:** each time a dragon bursts out of the sand, it throws out a ring of fire or ice.
- **Burning ground and ice spikes** linger for about 4 seconds after the big attacks; ice spikes freeze you once, then shatter.
- **Body:** the red dragon throws off flames and the blue one sheds freezing mist.
- **After one dies:** the survivor takes its twin's element too. Its breath splits into three jets (one element in the middle, the other on the sides, swapping each time), and its storms mix meteors and icicles.
- **With the Giant's Mask:** the dragons' attacks do the same damage as at normal size, they shrink less than the arena and move faster relative to it (so you can't just walk away), chase and attack about 25% more often, and touching a body hurts twice as much as before (still half of what it does at normal size).

#### Majora
- **Mask:** every 150 frames it shakes as a warning, then fires one of three volleys in turn: a fan of 5 aimed at you, an expanding ring of 12, or 6 orbs raining down around you.
- **Incarnation:** its pirouette sprays a two-armed spiral of orbs, its moonwalk leaves slow drifting orbs behind, and its energy-ball attack throws two extra side shots with each ball.
- **Wrath:** every whip crack sends a fan of 3 bolts along the floor from the whip tip, and its spin attack flings orbs in all directions.
- **The Remains return:** at ¾, ½ and ¼ health, Wrath calls the Remains back as flying enemies that shoot and ram you - first Odolwa and Goht, then Gyorg and Twinmold, then all four. They die when Wrath dies.
- **Fierce Deity:** Majora takes half damage, the Mask uses its fire beam (vanilla never does against Fierce Deity), and every pattern gets bigger: a fan of 7, a ring of 16, a four-armed spiral, 5 whip bolts, and all four Remains in every wave. This has its own switch.

#### Fixes
- A vanilla bug where the Remains read the Mask's freed memory after it turns into the Incarnation is fixed (the summoned Remains would hit it constantly).
- 2S2H's own **Boss Health Multiplier** turns itself off while Boss HP is on, since stacking them would wrap Wrath's 120 health around to a negative number.

### Miniboss & Enemy Overhaul
*Enhancements → Difficulty Options* - one switch per part (Miniboss HP, Miniboss Attacks, Eyegore, Deep Pythons, Them, Regular Enemies, More Encounters, Elite Enemies, Final Day Frenzy...)

#### Minibosses (2× health unless noted)
- **Gekko & Snapper:** while crawling on the walls, Gekko lobs mud bubbles that knock you back. Snapper's rolling charge ends in a stomp shockwave (clear it with a sidehop or backflip).
- **Gekko & Mad Jelly:** the reformed jelly drops from the ceiling as a shockwave, mini jellies hop at you, and shattering the frozen jelly throws ice shards to shield against.
- **Wart:** in the bubble phase the eye glows, then fires a short sweeping laser, and bubbles launch in rings. In the charging phase every wall slam rains bubbles around the room.
- **Captain Keeta:** his big slam sends a shockwave up the path and raises 2 Stalchildren near you. When he stops to fight, he adds a leaping spin slash.
- **Iron Knuckle:** the overhead smash sends a shockwave forward, the armor bursts off as shrapnel, and without armor it sometimes throws its axe like a boomerang - while the axe is out, it's unarmed and open.
- **Igos du Ikana & lackeys:** the fire breath leaves burning patches, the flying head spits fireballs, a lackey not finished with fire or light gets back up faster, and from half health the lackeys fight in tandem (one blocks while the other flanks).
- **Garo Master:** flaming swords send fire waves along the floor, the teleport-drop lands in a ring of flames, at half health it calls in 2 Garos, and the death explosion throws embers first as a warning.
- **Gomess:** bats break off the swarm to dive at you in waves, the spin throws the scythe out and back, and at low health it fades out and reappears behind you.
- **Eyegore** (about 2× harder): 16 health instead of 8. Its laser fires 5 shots that step towards you with a lag (once enraged the last becomes a 2-second sweeping beam; the Mirror Shield still cancels it). Its slam sends a shockwave ring and rains rubble. Enraged at half health: red eye, 25% faster, slams twice and also hits behind itself. New opening: an arrow in the eye while it charges the laser makes it stagger.
- **Deep Pythons:** 8 health instead of 4. They lunge and bite without grabbing first and spit water shots; when hurt, a python retreats and can burst out of a different, emptied burrow later. The last one alive becomes a real miniboss with its own music, 3× health and bigger attacks. With **Deep Pythons Swim Free** they also leave their burrows and hunt you through open water.
- **Them** (the ranch aliens): they weave as they come and about half blink out for a moment. After a few die, a bigger leader can appear (two arrows, extra red rupee). Aliens near you fire slow balls of light worth half a heart, and the light over the barn drops marked light bombs about every 10 seconds. They still reach the barn at exactly the same time, so winning and losing work as before.

#### Regular enemies (mostly new behavior, not more health)
- **Big Octo:** fires spreads of rocks at range, dives and resurfaces elsewhere after a hit, and calls 2 Octoroks at half health.
- **Boe (black & white):** groups pounce together; 4 or more that touch merge into a Big Boe that splits when hit. A White Boe's pounce can freeze you.
- **Dinolfos:** the fire breath sweeps side to side, it tail-spins if you get behind it, and pairs team up (one breathes while the other jump-slashes from the side).
- **Eeno:** big ones roll a growing snowball, small ones throw from three sides, and merging slams down a snow shockwave.
- **Wolfos / White Wolfos:** feint a lunge then sidestep, circle instead of attacking your raised shield, howl once per fight for help; a White Wolfos slash can freeze.
- **Death Armos:** the crash-down sends a shockwave ring, one that spots you wakes the others nearby, and its death dive homes in before exploding.
- **ReDead / Gibdo:** get back up once unless finished with fire; a ReDead scream calls the others in the room; a burned Gibdo loses its bandages and fights on as a ReDead.
- **Poe Sisters:** Meg's clones all attack at once, Jo leaves fire trails, Beth throws a freezing blue flame, Amy vanishes and reappears behind you. Their health stays the same because of the 3-minute timer.

#### More encounters
- Enemies sometimes bring companions of their own kind, and some areas add a local creature (Keese with Chuchus on the night fields, Dragonflies over the swamp Octoroks, Ice Keese with Eenos, Bubbles with Ikana's Stalchildren).
- Random encounters stay regional: Dragonflies, Octoroks and Mad Scrubs in the swamp; White Wolfos pairs and Eenos with White Boe in the mountains; Skullfish packs led by a Desbreko and bigger Leever rings in Great Bay; chains of 2-3 Garos and bigger Stalchild groups in Ikana.
- **In dungeons**, companions only appear in rooms where no door or chest waits on killing every enemy. Boss rooms, minigames, the Moon, Spider Houses, grottos and the Secret Shrine are left alone.
- **Elites:** about 1 enemy in 20 is bigger, sparkles gold, has double health and drops something better.
- **Final Day Frenzy:** companions are twice as common on the third day and three times as common after midnight, when elites are also twice as common.

### Sword Projectiles
*Enhancements → Items/Songs → Sword Projectiles*

| Sword | When | Projectile | Damage |
| - | - | - | - |
| **Gilded Sword** | Full hearts | Golden crescent, fast, disappears on the first hit | 3, same as a Gilded Sword slash |
| **Great Fairy's Sword** | Full hearts | Big pink spinning ring, slower, cuts through every enemy in its path | 4 to each enemy |
| **Fierce Deity** | Any health | Glowing orb, the fastest, bursts on impact and hits everything around it | 6, plus 4 from the burst |

The Fierce Deity orb replaces the old sword beam, fires on any normal slash (even without Z-targeting), and costs 1 magic like the old beam; with an empty magic meter, no orb comes out.

### Epona Control Overhaul
*Enhancements → Epona*

- **Control Mode**
  - **Tank**: the original controls.
  - **Freeform** *(default)*: Epona runs wherever you point the stick, relative to the camera, just like Link on foot. Letting go of the stick brakes, and she never backs up by herself.
  - **Hybrid**: Freeform, but switches to Tank while you aim in first person (the bow from horseback).
- **Handling sliders**: turn and brake strength for each mode, plus a minimum turn speed for Freeform.
- **Bunny Hood Boost**: wearing the Bunny Hood doubles Epona's speed and turn speed.
- Untick **Enable Epona Control Overhaul** for exactly the original riding.

### Swimming Overhaul
*Enhancements → Swimming*

| Option | What it does |
| - | - |
| **Swim Speed** | 1.0x to 2.0x at the surface (default 1.3x) |
| **Smooth Strokes** | Link speeds up steadily while you hold the stick, instead of surging on each stroke and slowing in between |
| **Faster Turning** | Turns about 75% faster, and stops three times quicker when you reverse direction |
| **Faster Diving** | Sinks about twice as fast while holding A, with full steering underwater instead of half |
| **Faster Resurfacing** | Rises back to the surface about 50% faster |
| **Zora Sharper Turning** | During Zora Link's fast swim (holding A), turns about 75% faster |
| **Zora Quick Dives** | During the fast swim, Zora Link tilts down to dive and up to surface about twice as fast |

How deep Link can dive doesn't change, and 2S2H's own **Faster Swim** (mashing B) still works alongside. Untick **Swimming Overhaul** for exactly the original swimming.

### Instant Text
*Enhancements → Time Savers → Instant Text*

- Each text box appears complete at once instead of being typed out. You still press A to go to the next box.
- Timed pauses in the middle of a box still pause, sounds inside the text still play, and Ocarina prompts and choices work as usual.
- Untick **Instant Text** to go back to the normal typing effect (or 2S2H's own **Fast Text**).

---

## How to play (Windows)

### 1. Get the game
Open the [**Actions**](https://github.com/GiorgosXaral/2S2H-MM-Overhaul/actions/workflows/build-overhaul.yml) tab, click the latest successful **build-2s2h-mm-overhaul** run, and download **2s2h-mm-overhaul** from the *Artifacts* section at the bottom (you need to be signed in to GitHub).

The download is a complete game folder. You don't need to install 2 Ship 2 Harkinian separately.

### 2. Check your ROM
You need your own copy of Majora's Mask. Check that your ROM dump is supported with the [compatibility checker](https://2ship.equipment/), or compare its `sha1` hash with the ones in [docs/supportedHashes.json](docs/supportedHashes.json).

### 3. Launch
- Unzip the download into its own folder.
- Run `2ship.exe` and select your ROM when asked. On first launch the game builds `mm.o2r` from your ROM; after that it starts straight away.

**Coming from 2S2H M3.1.0?** Copy your save files and `2ship2harkinian.json` from your old folder into the new one to keep your saves and settings.

---

## Controls

### Default keyboard configuration
| N64 | A | B | Z | Start | Analog stick | C buttons | D-Pad |
| - | - | - | - | - | - | - | - |
| Keyboard | X | C | Z | Space | WASD | Arrow keys | TFGH |

### Other shortcuts
| Keys | Action |
| - | - |
| F1 | Toggle menu bar |
| F11 | Fullscreen |
| Tab | Toggle Alternate assets |
| Ctrl+R | Reset |

### Graphics backends
DirectX 11 is the default on Windows; OpenGL is also available. You can switch in the **Settings** menu (requires a restart). If the game crashes, open `2ship2harkinian.json`, find the `"Backend": {` section and set the backend ID to `1` (OpenGL).

## Custom assets
Custom assets are packed in `.o2r` or `.otr` files. To use them, place them in the `mods` folder. To make your own, see [retro](https://github.com/HarbourMasters64/retro) (OTR and O2R generator) and [fast64](https://github.com/HarbourMasters/fast64) (Blender plugin; Majora's Mask isn't fully supported yet).

---

## Building
Every push to `main` builds a complete Windows game folder automatically with GitHub Actions ([`.github/workflows/build-overhaul.yml`](.github/workflows/build-overhaul.yml)).

To build it yourself, follow the original [building instructions](docs/BUILDING.md).

## Credits
- **2 Ship 2 Harkinian** by [HarbourMasters](https://github.com/HarbourMasters/2ship2harkinian), built on [libultraship](https://github.com/Kenix3/libultraship) and the Majora's Mask decompilation project.
- **Mouse support** by [lightmanLP](https://github.com/lightmanLP/2ship2harkinian).
- **Epona Control Overhaul** originally by LT_Schmiddy for *Majora's Mask: Recompiled*.
- Original 2 Ship 2 Harkinian [Discord](https://discord.com/invite/shipofharkinian) (for questions about the original game, not this modified version).

More documentation: [Custom Sequenced Music](docs/CUSTOM_SEQUENCED_MUSIC.md) · [Custom Streamed Music](docs/CUSTOM_STREAMED_MUSIC.md)
