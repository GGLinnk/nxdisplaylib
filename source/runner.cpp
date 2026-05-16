#include "nxdisplaylib/runner.hpp"
#include <cstdio>

namespace nxd {

bool Runner::init(View** views, int count, const RunnerConfig& cfg, int start) {
    if (!gfx_.init())
        return false;

    // A single player reading handheld + standard controllers.
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad_);
    hidInitializeTouchScreen();
    if (cfg.initGesture)
        hidInitializeGesture();

    views_ = views;
    count_ = count;
    cfg_   = cfg;
    current_ = (start >= 0 && start < count) ? start : 0;
    views_[current_]->onEnter();

    lastTick_ = armGetSystemTick();
    return true;
}

void Runner::deinit() {
    gfx_.deinit();
}

void Runner::switchTo(int index) {
    if (index < 0 || index >= count_ || index == current_) return;
    views_[current_]->onExit();
    current_ = index;
    View* v = views_[current_];
    v->requestIndex = -1;
    v->onEnter();
}

void Runner::drawChrome(View* v) {
    const u32 barBg  = Gfx::rgb(18, 20, 28);
    const u32 fg     = Gfx::rgb(232, 234, 244);
    const u32 accent = Gfx::rgb(120, 180, 255);
    const u32 shadow = Gfx::rgb(0, 0, 0);
    const int topH = 30, botH = 30;
    const int botY = Gfx::H - botH + 7;

    char fps[32] = {};
    int  fpsX = Gfx::W;
    if (cfg_.showFps) {
        snprintf(fps, sizeof(fps), "FPS %.1f", fpsValue_);
        fpsX = Gfx::W - gfx_.textWidth(2, fps) - 12;
    }

    if (v->transparentChrome()) {
        // No solid bars: shadowed text so the underlying view shows through.
        gfx_.drawTextShadow(12, 7, 2, accent, shadow, v->name());
        if (cfg_.showFps) gfx_.drawTextShadow(fpsX, 7, 2, fg, shadow, fps);
        gfx_.drawTextShadow(12, botY, 2, fg, shadow, v->controls());
    } else {
        gfx_.fillRect(0, 0, Gfx::W, topH, barBg);
        gfx_.drawText(12, 7, 2, accent, v->name());
        if (cfg_.showFps) gfx_.drawText(fpsX, 7, 2, fg, fps);

        gfx_.fillRect(0, Gfx::H - botH, Gfx::W, botH, barBg);
        gfx_.drawText(12, botY, 2, fg, v->controls());
    }
}

void Runner::run() {
    const int cycle = (cfg_.cycleCount > 0 && cfg_.cycleCount <= count_)
                          ? cfg_.cycleCount : count_;

    while (appletMainLoop()) {
        // --- timing ---
        u64 now = armGetSystemTick();
        double dt = (double)armTicksToNs(now - lastTick_) / 1.0e9;
        lastTick_ = now;
        if (dt > 0.25) dt = 0.25;   // clamp after suspend / long stalls

        fpsAcc_ += dt;
        fpsFrames_++;
        if (fpsAcc_ >= 0.5) {
            fpsValue_  = fpsFrames_ / fpsAcc_;
            fpsAcc_    = 0.0;
            fpsFrames_ = 0;
        }

        // --- input ---
        padUpdate(&pad_);
        Input in;
        in.down   = padGetButtonsDown(&pad_);
        in.up     = padGetButtonsUp(&pad_);
        in.held   = padGetButtons(&pad_);
        in.lstick = padGetStickPos(&pad_, 0);
        in.rstick = padGetStickPos(&pad_, 1);
        in.dtSec  = dt;
        hidGetTouchScreenStates(&in.touch, 1);

        // --- global navigation ---
        View* v = views_[current_];
        if (!v->capturesExit() && (in.down & HidNpadButton_Plus))
            break;  // return to hbmenu

        if (!v->capturesCycle()) {
            if (in.down & HidNpadButton_ZR)
                switchTo((current_ + 1) % cycle);
            else if (in.down & HidNpadButton_ZL)
                switchTo((current_ + cycle - 1) % cycle);
        }
        if (!v->capturesExit() && (in.down & HidNpadButton_B)) {
            if (current_ != cfg_.homeIndex)
                switchTo(cfg_.homeIndex);
            else if (cfg_.exitOnHomeBack)
                break;   // B on the home view: leave the app
        }

        // --- update active view ---
        v = views_[current_];
        v->update(in);
        if (v->requestIndex >= 0) {
            int req = v->requestIndex;
            v->requestIndex = -1;
            switchTo(req);
            v = views_[current_];
        }

        // --- render ---
        gfx_.beginFrame();
        v->render(gfx_);
        if (v->showChrome())
            drawChrome(v);
        v->renderOverlay(gfx_);   // drawn on top of the chrome bars
        gfx_.endFrame();
    }
}

} // namespace nxd
