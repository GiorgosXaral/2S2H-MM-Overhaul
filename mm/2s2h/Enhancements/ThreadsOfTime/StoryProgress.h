#pragma once

// 2S2H-MM-Overhaul: the story of Majora's Mask as a list of milestones, each with step-by-step directions,
// worked out from the save file. Majora's Mask resets most progress when you play the Song of Time, so every
// step says whether it is permanent (masks, songs, items, Remains) or has to be redone each cycle.
// Used by the Threads of Time window, and meant to be shared with Tatl's hints later.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace StoryProgress {

struct Step {
    std::string text;
    // Optional: when set, the step shows a tick once it returns true. Steps that leave no trace in the save
    // have no check and are shown as plain directions.
    std::function<bool()> done;
    // True when the Song of Time undoes this step, so it has to be done again in a new cycle.
    bool eachCycle;
};

struct Milestone {
    std::string title;  // the real name, shown once the milestone is reached
    std::string veiled; // a spoiler-free hint, shown for milestones still ahead
    std::string where;  // the main area
    std::vector<Step> steps;
    // Only permanent progress (Remains, masks, songs, items) completes a milestone, so a milestone never
    // "un-completes" after the Song of Time.
    std::function<bool()> done;
};

// A side quest: always shown in full, since it doesn't spoil the story.
struct SideThread {
    std::string title;
    std::string note; // shown above the steps (when to start, what it gives)
    std::vector<Step> steps;
    std::function<bool()> done;
};

const std::vector<Milestone>& MainMilestones();
const std::vector<SideThread>& SideThreads();

// Index of the first main milestone not done yet (the final battle never completes in the save).
int32_t CurrentMilestoneIndex();

} // namespace StoryProgress
