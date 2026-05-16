#pragma once
#include "nxdisplaylib/input.hpp"

namespace nxd {

class Gfx;

// A navigable screen. The Runner drives update() then render() each frame.
// Views refer to one another by index into the Runner's view list, so each
// application keeps its own screen enumeration.
class View {
public:
    virtual ~View() {}

    virtual void onEnter() {}
    virtual void onExit()  {}
    virtual void update(const Input& in) = 0;
    virtual void render(Gfx& g) = 0;
    // Drawn after the Runner chrome, so it sits on top of the bars.
    virtual void renderOverlay(Gfx& g) { (void)g; }

    virtual const char* name() const = 0;
    virtual const char* controls() const { return ""; }

    // When false, the Runner suppresses its top/bottom chrome this frame.
    virtual bool showChrome() const { return true; }
    // When true, the Runner draws chrome text without the solid bars, so the
    // underlying view shows through.
    virtual bool transparentChrome() const { return false; }
    // When true, the Runner leaves ZL/ZR to the view instead of cycling views.
    virtual bool capturesCycle() const { return false; }
    // When true, the Runner leaves B and + to the view: it neither returns to
    // the home view on B nor exits on +, so the view owns its way out.
    virtual bool capturesExit() const { return false; }

    // A view sets requestIndex to ask the Runner to switch; the Runner clears
    // it. -1 means "no request".
    int  requestIndex = -1;
    void requestSwitch(int index) { requestIndex = index; }
};

} // namespace nxd
