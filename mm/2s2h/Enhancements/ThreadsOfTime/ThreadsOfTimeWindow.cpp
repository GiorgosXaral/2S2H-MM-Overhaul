#include "ThreadsOfTimeWindow.h"
#include "StoryProgress.h"

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <ship/Context.h>
#include <ship/window/Window.h>
#include <fast/Fast3dGui.h>

extern "C" {
#include "z64.h"
#include "macros.h"
#include "variables.h"
extern PlayState* gPlayState;
}

using namespace StoryProgress;

#define TOT_CVAR_WHOLE_ROAD "gEnhancements.ThreadsOfTime.ShowWholeRoad"

static const ImVec4 kColorDone = ImVec4(0.55f, 0.85f, 0.55f, 1.0f);
static const ImVec4 kColorNow = ImVec4(1.0f, 0.85f, 0.35f, 1.0f);
static const ImVec4 kColorAhead = ImVec4(0.55f, 0.55f, 0.6f, 1.0f);
static const ImVec4 kColorDim = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
static const ImVec4 kColorCycle = ImVec4(0.55f, 0.75f, 1.0f, 1.0f);

static bool IsSaveLoaded() {
    return (gPlayState != NULL) && (gSaveContext.gameMode == GAMEMODE_NORMAL);
}

static bool Has(int32_t item) {
    return INV_CONTENT(item) == item;
}

// ---- Tapestry ---------------------------------------------------------------------------------

struct TapestryIcon {
    int32_t item;
    bool owned;
    const char* name;
};

static void DrawIconRow(const std::vector<TapestryIcon>& icons, bool revealAll) {
    const float size = 36.0f;
    auto gui = std::dynamic_pointer_cast<Fast::Fast3dGui>(Ship::Context::GetRawInstance()->GetWindow()->GetGui());

    for (size_t i = 0; i < icons.size(); i++) {
        const TapestryIcon& icon = icons[i];
        if (i > 0) {
            ImGui::SameLine();
        }
        // Not yet owned: drawn dark and faded
        const ImVec4 tint = icon.owned ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.25f, 0.25f, 0.25f, 0.6f);
        ImGui::Image(gui->GetTextureByName((const char*)gItemIcons[icon.item]), ImVec2(size, size), ImVec2(0, 0),
                     ImVec2(1, 1), tint, ImVec4(0, 0, 0, 0));
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", (icon.owned || revealAll) ? icon.name : "???");
        }
    }
}

static void DrawTapestry(bool revealAll) {
    ImGui::SeparatorText("The Tapestry");

    DrawIconRow({ { ITEM_REMAINS_ODOLWA, CHECK_QUEST_ITEM(QUEST_REMAINS_ODOLWA) != 0, "Odolwa's Remains" },
                  { ITEM_REMAINS_GOHT, CHECK_QUEST_ITEM(QUEST_REMAINS_GOHT) != 0, "Goht's Remains" },
                  { ITEM_REMAINS_GYORG, CHECK_QUEST_ITEM(QUEST_REMAINS_GYORG) != 0, "Gyorg's Remains" },
                  { ITEM_REMAINS_TWINMOLD, CHECK_QUEST_ITEM(QUEST_REMAINS_TWINMOLD) != 0, "Twinmold's Remains" } },
                revealAll);
    ImGui::SameLine(0.0f, 24.0f);
    DrawIconRow({ { ITEM_MASK_DEKU, Has(ITEM_MASK_DEKU), "Deku Mask" },
                  { ITEM_MASK_GORON, Has(ITEM_MASK_GORON), "Goron Mask" },
                  { ITEM_MASK_ZORA, Has(ITEM_MASK_ZORA), "Zora Mask" },
                  { ITEM_MASK_FIERCE_DEITY, Has(ITEM_MASK_FIERCE_DEITY), "Fierce Deity's Mask" } },
                revealAll);

    DrawIconRow({ { ITEM_OCARINA_OF_TIME, Has(ITEM_OCARINA_OF_TIME), "Ocarina of Time" },
                  { ITEM_BOW, Has(ITEM_BOW), "Hero's Bow" },
                  { ITEM_LENS_OF_TRUTH, Has(ITEM_LENS_OF_TRUTH), "Lens of Truth" },
                  { ITEM_HOOKSHOT, Has(ITEM_HOOKSHOT), "Hookshot" },
                  { ITEM_ARROW_FIRE, Has(ITEM_ARROW_FIRE), "Fire Arrows" },
                  { ITEM_ARROW_ICE, Has(ITEM_ARROW_ICE), "Ice Arrows" },
                  { ITEM_ARROW_LIGHT, Has(ITEM_ARROW_LIGHT), "Light Arrows" },
                  { ITEM_MASK_GIANT, Has(ITEM_MASK_GIANT), "Giant's Mask" } },
                revealAll);
}

// ---- Steps ------------------------------------------------------------------------------------

// Steps are in order, so everything up to the last step with a tick is done too, and the step right after
// it is what to do next.
static void DrawSteps(const std::vector<Step>& steps, bool allDone, bool highlightNext) {
    int32_t lastDone = -1;
    if (allDone) {
        lastDone = static_cast<int32_t>(steps.size()) - 1;
    } else {
        for (size_t i = 0; i < steps.size(); i++) {
            if (steps[i].done && steps[i].done()) {
                lastDone = static_cast<int32_t>(i);
            }
        }
    }

    for (size_t i = 0; i < steps.size(); i++) {
        const Step& step = steps[i];
        const bool done = static_cast<int32_t>(i) <= lastDone;
        const bool isNext = highlightNext && (static_cast<int32_t>(i) == lastDone + 1);

        ImGui::PushID(static_cast<int>(i));
        if (step.done) {
            bool shown = done;
            ImGui::BeginDisabled();
            ImGui::Checkbox("##step", &shown);
            ImGui::EndDisabled();
            ImGui::SameLine();
        } else {
            ImGui::Bullet();
        }

        if (step.eachCycle) {
            ImGui::TextColored(kColorCycle, "[each cycle]");
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("The Song of Time undoes this: do it again in each new cycle where you need it.");
            }
            ImGui::SameLine();
        }

        ImGui::PushStyleColor(ImGuiCol_Text,
                              done ? kColorDim : (isNext ? kColorNow : ImGui::GetStyleColorVec4(ImGuiCol_Text)));
        if (isNext) {
            ImGui::TextWrapped("Next: %s", step.text.c_str());
        } else {
            ImGui::TextWrapped("%s", step.text.c_str());
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }
}

// ---- This cycle -------------------------------------------------------------------------------

static void DrawThisCycle() {
    ImGui::SeparatorText("This Cycle");

    const int32_t day = CURRENT_DAY;
    const int32_t minutes = static_cast<int32_t>(TIME_TO_MINUTES_F((u16)CURRENT_TIME));
    const int32_t hour = (minutes / 60) % 24;
    const int32_t minute = minutes % 60;
    const char* dayName = (day <= 1) ? "Dawn of the First Day" : (day == 2) ? "the Second Day" : "the Final Day";

    ImGui::Text("Day %d (%s), %d:%02d %s", (day < 1) ? 1 : ((day > 3) ? 3 : day), dayName,
                ((hour % 12) == 0) ? 12 : (hour % 12), minute, (hour < 12) ? "AM" : "PM");

    if ((day >= 1) && (day <= 3)) {
        const int32_t left = static_cast<int32_t>(TIME_TO_MINUTES_F((u32)TIME_UNTIL_MOON_CRASH));
        ImGui::PushStyleColor(ImGuiCol_Text, (left < 6 * 60) ? ImVec4(1.0f, 0.45f, 0.4f, 1.0f) : kColorDim);
        ImGui::Text("The moon falls in %d hours %02d minutes (game time).", left / 60, left % 60);
        ImGui::PopStyleColor();
    }
    ImGui::TextDisabled("Bank: %d Rupees. Rupees in your wallet, ammo, keys and bottled things are lost when you "
                        "play the Song of Time.",
                        HS_GET_BANK_RUPEES());
}

// ---- Next objective ---------------------------------------------------------------------------

static void DrawNextObjective() {
    ImGui::SeparatorText("Next Objective");

    const int32_t current = CurrentMilestoneIndex();
    if (current < 0) {
        ImGui::TextColored(kColorDone, "Termina is saved.");
        return;
    }

    const Milestone& milestone = MainMilestones()[current];
    ImGui::TextColored(kColorNow, "%s", milestone.title.c_str());
    ImGui::TextDisabled("Where: %s", milestone.where.c_str());
    ImGui::Spacing();
    DrawSteps(milestone.steps, false, true);
}

// ---- Timeline ---------------------------------------------------------------------------------

static void DrawTimeline(bool revealAll) {
    ImGui::SeparatorText("The Road");

    const auto& milestones = MainMilestones();
    const int32_t current = CurrentMilestoneIndex();

    for (size_t i = 0; i < milestones.size(); i++) {
        const Milestone& milestone = milestones[i];
        const bool isDone = milestone.done();
        const bool isNow = static_cast<int32_t>(i) == current;
        const bool revealed = isDone || isNow || revealAll;

        std::string label;
        ImVec4 color;
        if (isDone) {
            label = "Done:  " + milestone.title;
            color = kColorDone;
        } else if (isNow) {
            label = "Now:   " + milestone.title;
            color = kColorNow;
        } else {
            label = "Ahead: " + (revealAll ? milestone.title : milestone.veiled);
            color = kColorAhead;
        }
        label += "###milestone" + std::to_string(i);

        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
        if (!revealed) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        }

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        const bool open = ImGui::TreeNodeEx(label.c_str(), flags);
        ImGui::PopStyleColor();

        if (revealed && open) {
            ImGui::TextDisabled("Where: %s", milestone.where.c_str());
            DrawSteps(milestone.steps, isDone, isNow);
            ImGui::TreePop();
        }
    }
}

// ---- Side threads -----------------------------------------------------------------------------

static void DrawSideThreads() {
    ImGui::SeparatorText("Side Threads");

    const auto& threads = SideThreads();
    for (size_t i = 0; i < threads.size(); i++) {
        const SideThread& thread = threads[i];
        const bool done = thread.done();

        std::string label = (done ? "Done:  " : "") + thread.title + "###side" + std::to_string(i);
        ImGui::PushStyleColor(ImGuiCol_Text, done ? kColorDone : ImGui::GetStyleColorVec4(ImGuiCol_Text));
        const bool open = ImGui::TreeNodeEx(label.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth);
        ImGui::PopStyleColor();

        if (open) {
            if (!thread.note.empty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, kColorDim);
                ImGui::TextWrapped("%s", thread.note.c_str());
                ImGui::PopStyleColor();
            }
            // Side threads are lists of separate goals, so each step is ticked on its own
            for (size_t j = 0; j < thread.steps.size(); j++) {
                const Step& step = thread.steps[j];
                const bool stepDone = done || (step.done && step.done());

                ImGui::PushID(static_cast<int>(j));
                bool shown = stepDone;
                ImGui::BeginDisabled();
                ImGui::Checkbox("##side", &shown);
                ImGui::EndDisabled();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, stepDone ? kColorDim : ImGui::GetStyleColorVec4(ImGuiCol_Text));
                ImGui::TextWrapped("%s", step.text.c_str());
                ImGui::PopStyleColor();
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }
}

// ---- Collection -------------------------------------------------------------------------------

static void Row(const char* name, const char* fmt, ...) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::TextUnformatted(name);
    ImGui::TableNextColumn();
    char buf[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    ImGui::TextUnformatted(buf);
}

static void DrawCollection() {
    ImGui::SeparatorText("Collection");

    int32_t masks = 0;
    for (int32_t item = ITEM_MASK_DEKU; item <= ITEM_MASK_GIANT; item++) {
        masks += Has(item) ? 1 : 0;
    }

    static const int32_t kSongs[] = {
        QUEST_SONG_TIME,    QUEST_SONG_HEALING, QUEST_SONG_EPONA,      QUEST_SONG_SOARING, QUEST_SONG_STORMS,
        QUEST_SONG_SONATA,  QUEST_SONG_LULLABY, QUEST_SONG_BOSSA_NOVA, QUEST_SONG_ELEGY,   QUEST_SONG_OATH,
    };
    int32_t songs = 0;
    for (int32_t song : kSongs) {
        songs += CHECK_QUEST_ITEM(song) ? 1 : 0;
    }

    int32_t owls = 0;
    for (int32_t owl = OWL_WARP_GREAT_BAY_COAST; owl <= OWL_WARP_STONE_TOWER; owl++) {
        owls += GET_OWL_STATUE_ACTIVATED(owl) ? 1 : 0;
    }

    const int32_t hearts = gSaveContext.save.saveInfo.playerData.healthCapacity / 16;
    const int32_t pieces = GET_QUEST_HEART_PIECE_COUNT;
    const s8* fairies = gSaveContext.save.saveInfo.inventory.strayFairies;
    const u32 tokens = gSaveContext.save.saveInfo.skullTokenCount;

    if (ImGui::BeginTable("totCollection", 2, ImGuiTableFlags_SizingStretchProp)) {
        Row("Masks", "%d / 24", masks);
        Row("Hearts", "%d / 20   (+%d/4 pieces toward the next)", hearts, pieces);
        Row("Songs", "%d / 10", songs);
        Row("Owl Statues", "%d / 10", owls);
        Row("Bombers' Notebook", "%s", CHECK_QUEST_ITEM(QUEST_BOMBERS_NOTEBOOK) ? "Yes" : "Not yet (the Bombers' leader "
                                                                                      "in East Clock Town, as a human)");
        Row("Stray Fairies (this cycle)", "Woodfall %d/15, Snowhead %d/15, Great Bay %d/15, Stone Tower %d/15",
            fairies[DUNGEON_SCENE_INDEX_WOODFALL_TEMPLE], fairies[DUNGEON_SCENE_INDEX_SNOWHEAD_TEMPLE],
            fairies[DUNGEON_SCENE_INDEX_GREAT_BAY_TEMPLE], fairies[DUNGEON_SCENE_INDEX_STONE_TOWER_TEMPLE]);
        Row("Gold Skulltulas (this cycle)", "Swamp %u/30, Ocean %u/30", (tokens >> 16) & 0xFFFF, tokens & 0xFFFF);
        ImGui::EndTable();
    }
}

// ---- Window -----------------------------------------------------------------------------------

void ThreadsOfTimeWindow::DrawElement() {
    if (!IsSaveLoaded()) {
        ImGui::TextWrapped("Load a save file to see your journey.");
        return;
    }

    if (gSaveContext.save.shipSaveInfo.saveType == SAVETYPE_RANDO) {
        ImGui::PushStyleColor(ImGuiCol_Text, kColorAhead);
        ImGui::TextWrapped("Threads of Time follows the original story. In a Randomizer save, items are in other "
                           "places, so the directions won't match.");
        ImGui::PopStyleColor();
    }

    // Everything here gets its own ID scope: the menu can draw this window inside itself, next to other widgets
    ImGui::PushID("ThreadsOfTimeWindow");

    bool revealAll = CVarGetInteger(TOT_CVAR_WHOLE_ROAD, 0) != 0;
    if (ImGui::Checkbox("Show the whole road##ThreadsOfTime", &revealAll)) {
        CVarSetInteger(TOT_CVAR_WHOLE_ROAD, revealAll ? 1 : 0);
        Ship::Context::GetRawInstance()->GetWindow()->GetGui()->SaveConsoleVariablesNextFrame();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reveal the names of every milestone still ahead (spoilers).");
    }

    DrawTapestry(revealAll);
    DrawThisCycle();
    DrawNextObjective();
    DrawTimeline(revealAll);
    DrawSideThreads();
    DrawCollection();

    ImGui::PopID();
}
