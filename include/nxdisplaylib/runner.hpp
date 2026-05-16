#pragma once
#include "nxdisplaylib/gfx.hpp"
#include "nxdisplaylib/view.hpp"

namespace nxd {

// Behaviour knobs for the Runner.
struct RunnerConfig {
    int  homeIndex     = 0;   // view B returns to
    int  cycleCount    = 0;   // ZL/ZR cycle spans indices [0,cycleCount); 0 => all
    bool showFps       = false; // draw a live render-rate readout in the top bar
    bool initGesture   = false; // also bring up the HID gesture subsystem
    bool exitOnHomeBack= false; // B on the home view exits (else it does nothing)
};

// Hosts a set of Views: owns the renderer and input device, runs the main
// loop, dispatches input, handles navigation (ZL/ZR cycle, B-home, +-exit)
// and draws the shared chrome (top bar with the view name + optional FPS,
// bottom bar with the view's controls).
//
// run() is the live loop. step() / select() expose one frame at a time so a
// recorder or test harness can drive the views headlessly.
class Runner {
public:
    // `views` must outlive the Runner. Returns false if the framebuffer fails.
    bool init(View** views, int count, const RunnerConfig& cfg, int start = 0);
    void run();
    void deinit();

    // Render exactly one frame for `in` (navigation, update, render, chrome);
    // returns the rendered framebuffer (Gfx::W x Gfx::H), valid until the next
    // frame. The live loop is just step() fed polled input.
    const u32* step(const Input& in);
    // Switch the active view by index (runs onExit / onEnter).
    void select(int index);

    View* view(int i)   { return (i >= 0 && i < count_) ? views_[i] : nullptr; }
    int   count() const { return count_; }
    int   current() const { return current_; }

private:
    void drawChrome(View* v);

    Gfx          gfx_;
    PadState     pad_{};
    View**       views_   = nullptr;
    int          count_   = 0;
    int          current_ = 0;
    bool         exitRequested_ = false;
    RunnerConfig cfg_;

    // Render-rate measurement (rolling average over ~0.5 s).
    double fpsAcc_    = 0.0;
    int    fpsFrames_ = 0;
    double fpsValue_  = 0.0;
    u64    lastTick_  = 0;
};

// Write a Gfx::W x Gfx::H RGBA framebuffer as a binary PPM image. Returns
// false on an I/O error. The lib's frame-capture primitive.
bool savePpm(const char* path, const u32* pixels);

} // namespace nxd
