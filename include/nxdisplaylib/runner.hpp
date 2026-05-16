#pragma once
#include "nxdisplaylib/gfx.hpp"
#include "nxdisplaylib/view.hpp"

namespace nxd {

// Behaviour knobs for the Runner.
struct RunnerConfig {
    int  homeIndex   = 0;     // view B returns to
    int  cycleCount  = 0;     // ZL/ZR cycle spans indices [0,cycleCount); 0 => all
    bool showFps     = false; // draw a live render-rate readout in the top bar
    bool initGesture = false; // also bring up the HID gesture subsystem
};

// Hosts a set of Views: owns the renderer and input device, runs the main
// loop, dispatches input, handles navigation (ZL/ZR cycle, B-home, +-exit)
// and draws the shared chrome (top bar with the view name + optional FPS,
// bottom bar with the view's controls).
class Runner {
public:
    // `views` must outlive the Runner. Returns false if the framebuffer fails.
    bool init(View** views, int count, const RunnerConfig& cfg, int start = 0);
    void run();
    void deinit();

    View* view(int i)   { return (i >= 0 && i < count_) ? views_[i] : nullptr; }
    int   count() const { return count_; }

private:
    void switchTo(int index);
    void drawChrome(View* v);

    Gfx          gfx_;
    PadState     pad_{};
    View**       views_   = nullptr;
    int          count_   = 0;
    int          current_ = 0;
    RunnerConfig cfg_;

    // Render-rate measurement (rolling average over ~0.5 s).
    u64    lastTick_  = 0;
    double fpsAcc_    = 0.0;
    int    fpsFrames_ = 0;
    double fpsValue_  = 0.0;
};

} // namespace nxd
