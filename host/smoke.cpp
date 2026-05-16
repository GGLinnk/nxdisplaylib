// Framework smoke test: a tiny program that exercises the whole nxdisplaylib
// view framework on the host - the Runner loop, View navigation, the chrome,
// and both widgets (ListMenu, ScrollView). Build and run via the Makefile.
#include "nxdisplaylib/runner.hpp"
#include "nxdisplaylib/widgets.hpp"
#include <cstdio>

using namespace nxd;

// A scrollable content view, driven by nxd::ScrollView.
class PageView : public View {
public:
    PageView(const char* nm, u32 col) : nm_(nm), col_(col) {}

    void update(const Input& in) override { scroll_.handleInput(in); }

    void render(Gfx& g) override {
        const int top = 30, h = Gfx::H - 60;
        g.fillRect(0, top, Gfx::W, h, Gfx::rgb(16, 18, 26));
        const int rows = 50;
        scroll_.setMetrics(rows * 30, h);
        int off = scroll_.offset();
        for (int i = 0; i < rows; i++) {
            int y = top + i * 30 - off;
            if (y < top - 30 || y > top + h) continue;
            char buf[64];
            snprintf(buf, sizeof(buf), "%s  -  scrollable line %d", nm_, i);
            g.drawText(24, y + 4, 2, col_, buf);
        }
        scroll_.drawScrollbar(g, 0, top, Gfx::W, h);
    }

    const char* name() const override { return nm_; }
    const char* controls() const override {
        return "Up/Down/Stick/Drag: scroll   ZL/ZR: view   B: menu";
    }

private:
    const char* nm_;
    u32         col_;
    ScrollView  scroll_;
};

// A landing menu, driven by nxd::ListMenu.
class SmokeMenu : public View {
public:
    void update(const Input& in) override {
        ListMenu::Layout lay;
        lay.x = 120; lay.y = 150; lay.rowPitch = 84;
        lay.rowW = 1040; lay.rowVis = 68; lay.perPage = 6;
        menu_.configure(9, lay);
        menu_.handleInput(in);
        int a = menu_.activated();
        if (a >= 0) requestSwitch(1 + (a % 3));   // open one of the page views
    }

    void render(Gfx& g) override {
        g.clear(Gfx::rgb(13, 15, 23));
        g.drawText(120, 54, 4, Gfx::rgb(120, 180, 255), "nxdisplaylib host smoke");
        g.drawText(120, 104, 2, Gfx::rgb(150, 156, 170),
                   "Runner + View + ListMenu + ScrollView, on the desktop");
        for (int r = 0; r < 6; r++) {
            int x, y, w, h;
            int it = menu_.visibleItem(r, x, y, w, h);
            if (it < 0) break;
            bool sel = (it == menu_.selected());
            g.fillRect(x, y, w, h, sel ? Gfx::rgb(30, 52, 86) : Gfx::rgb(20, 23, 32));
            if (sel) g.drawRectThick(x, y, w, h, 2, Gfx::rgb(120, 180, 255));
            char buf[48];
            snprintf(buf, sizeof(buf), "Menu item %d", it);
            g.drawText(x + 18, y + 22, 3, Gfx::rgb(232, 234, 244), buf);
        }
    }

    const char* name() const override { return "Smoke Menu"; }
    const char* controls() const override {
        return "D-Pad: select   A: open   L/R: page   +: exit";
    }
    bool showChrome() const override { return false; }

private:
    ListMenu menu_;
};

int main() {
    static SmokeMenu menu;
    static PageView  alpha  ("Alpha",   Gfx::rgb(120, 224, 140));
    static PageView  bravo  ("Bravo",   Gfx::rgb(244, 208, 100));
    static PageView  charlie("Charlie", Gfx::rgb(244, 116, 110));

    static View* views[] = { &menu, &alpha, &bravo, &charlie };

    RunnerConfig cfg;
    cfg.homeIndex  = 0;
    cfg.cycleCount = 0;
    cfg.showFps    = true;

    Runner runner;
    if (!runner.init(views, 4, cfg)) {
        printf("nxdisplaylib host smoke: framebuffer init failed\n");
        return 1;
    }
    runner.run();
    runner.deinit();
    return 0;
}
