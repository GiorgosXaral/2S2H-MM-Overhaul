#pragma once

#include <ship/window/gui/GuiWindow.h>

// 2S2H-MM-Overhaul: Threads of Time, a window showing the journey so far, what to do next (step by step),
// what's still ahead (veiled unless "Show the whole road" is on), side quests and this cycle's clock.
class ThreadsOfTimeWindow : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;

    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};
