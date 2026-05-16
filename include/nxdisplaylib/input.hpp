#pragma once
#include <switch.h>

namespace nxd {

// Per-frame input snapshot the Runner hands to the active View.
struct Input {
    u64 down = 0;   // buttons newly pressed this frame
    u64 up   = 0;   // buttons newly released this frame
    u64 held = 0;   // buttons currently held
    HidTouchScreenState touch{};   // up to 16 simultaneous touch points
    HidAnalogStickState lstick{};  // left analog stick position
    HidAnalogStickState rstick{};  // right analog stick position
    double dtSec = 0.0;            // wall-clock seconds since the previous frame
};

} // namespace nxd
