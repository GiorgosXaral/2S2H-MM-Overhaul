#pragma once

#ifndef __cplusplus
#include <stdbool.h>
#endif

// 2S2H-MM-Overhaul: Giant's Mask and Fierce Deity's Mask always use magic (see MasksAlwaysUseMagic.cpp)

#ifdef __cplusplus
extern "C" {
#endif

struct Player;

// True while Link wears one of the two masks and the option is on (the Infinite Magic cheat then leaves magic alone).
bool MasksAlwaysUseMagic_Active(void);
// True when Fierce Deity Link has run out of magic and should turn back.
bool MasksAlwaysUseMagic_FierceDeityOutOfMagic(struct Player* player);

#ifdef __cplusplus
}
#endif
