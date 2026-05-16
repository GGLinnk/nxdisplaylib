// Host backend: implements the mock libnx surface (nxd_host_libnx.h) against
// SDL2 and the desktop clock, so the nxdisplaylib framework runs on a PC.
//
// An SDL window presents the framebuffer live; keyboard and mouse drive the
// pad and touchscreen. With no display available it falls back to headless
// rendering (useful for CI / frame dumps).
//
// Keyboard map:  arrows = D-pad    Z or Enter = A    X or Esc = B
//                A = Y   S = X     Q/W = L/R         1/2 = ZL/ZR
//                '=' = +  (exit)   RShift = -        window close = quit
// Mouse:         left button = touch
//
// Env vars:  NXD_HOST_FRAMES=N  auto-quit after N frames
//            NXD_HOST_DUMP=path write the final frame as a PPM image
#include "nxd_host_libnx.h"
#include <SDL2/SDL.h>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace {

constexpr int kW = 1280, kH = 720;

struct Host {
    SDL_Window*   window  = nullptr;
    SDL_Renderer* renderer= nullptr;
    SDL_Texture*  texture = nullptr;
    bool          headless = true;

    u32  buffer[kW * kH] = {};
    u64  cur = 0, prev = 0;          // button masks
    int  stickY = 0;                 // left-stick Y (driven by the mouse wheel)
    bool touchOn = false;
    int  touchX = 0, touchY = 0;

    bool quit = false;
    long frame = 0;
    long frameLimit = -1;            // -1 = unbounded
    const char* dumpPath = nullptr;

    // Scripted input (NXD_HOST_KEYS="frame:btn,...") for headless testing.
    struct ScriptKey { long frame; u64 btn; };
    ScriptKey script[32];
    int       scriptN = 0;

    std::chrono::steady_clock::time_point start;
    std::chrono::steady_clock::time_point lastPresent;
} g;

// Map a button name to its mask (a/b/x/y, l/r/zl/zr, up/down/left/right, +/-).
u64 buttonByName(const char* n) {
    if (!strcmp(n, "a")) return HidNpadButton_A;
    if (!strcmp(n, "b")) return HidNpadButton_B;
    if (!strcmp(n, "x")) return HidNpadButton_X;
    if (!strcmp(n, "y")) return HidNpadButton_Y;
    if (!strcmp(n, "l")) return HidNpadButton_L;
    if (!strcmp(n, "r")) return HidNpadButton_R;
    if (!strcmp(n, "zl")) return HidNpadButton_ZL;
    if (!strcmp(n, "zr")) return HidNpadButton_ZR;
    if (!strcmp(n, "up")) return HidNpadButton_Up;
    if (!strcmp(n, "down")) return HidNpadButton_Down;
    if (!strcmp(n, "left")) return HidNpadButton_Left;
    if (!strcmp(n, "right")) return HidNpadButton_Right;
    if (!strcmp(n, "plus")) return HidNpadButton_Plus;
    if (!strcmp(n, "minus")) return HidNpadButton_Minus;
    return 0;
}

void parseScript(const char* s) {
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", s);
    for (char* tok = strtok(buf, ","); tok && g.scriptN < 32; tok = strtok(nullptr, ",")) {
        char* colon = strchr(tok, ':');
        if (!colon) continue;
        *colon = 0;
        g.script[g.scriptN].frame = atol(tok);
        g.script[g.scriptN].btn   = buttonByName(colon + 1);
        g.scriptN++;
    }
}

void writePpm(const char* path) {
    FILE* f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", kW, kH);
    for (int i = 0; i < kW * kH; i++) {
        u32 p = g.buffer[i];                 // R | G<<8 | B<<16 | A<<24
        u8 rgb[3] = { (u8)(p & 0xff), (u8)((p >> 8) & 0xff), (u8)((p >> 16) & 0xff) };
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}

} // namespace

// --- window / framebuffer ---------------------------------------------------

NWindow* nwindowGetDefault(void) { static NWindow w; return &w; }

Result framebufferCreate(Framebuffer*, NWindow*, u32, u32, u32, u32) {
    g.start = std::chrono::steady_clock::now();
    if (const char* f = getenv("NXD_HOST_FRAMES")) g.frameLimit = atol(f);
    g.dumpPath = getenv("NXD_HOST_DUMP");
    if (const char* k = getenv("NXD_HOST_KEYS")) parseScript(k);

    // NXD_HOST_HEADLESS forces off-screen rendering (used by the asset
    // generator so it never pops a window).
    if (!getenv("NXD_HOST_HEADLESS") && SDL_Init(SDL_INIT_VIDEO) == 0) {
        g.window = SDL_CreateWindow("nxdisplaylib host",
                                    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    kW, kH, SDL_WINDOW_SHOWN);
        if (g.window) {
            g.renderer = SDL_CreateRenderer(g.window, -1,
                             SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            g.texture  = SDL_CreateTexture(g.renderer, SDL_PIXELFORMAT_ABGR8888,
                                           SDL_TEXTUREACCESS_STREAMING, kW, kH);
            g.headless = false;
        }
    }
    if (g.headless)
        printf("[nxd host] no display - running headless\n");
    return 0;
}

Result framebufferMakeLinear(Framebuffer*) { return 0; }

void framebufferClose(Framebuffer*) {
    if (g.dumpPath) writePpm(g.dumpPath);
    if (g.texture)  SDL_DestroyTexture(g.texture);
    if (g.renderer) SDL_DestroyRenderer(g.renderer);
    if (g.window)   SDL_DestroyWindow(g.window);
    if (!g.headless) SDL_Quit();
}

void* framebufferBegin(Framebuffer*, u32* out_stride) {
    if (out_stride) *out_stride = kW * sizeof(u32);
    return g.buffer;
}

void framebufferEnd(Framebuffer*) {
    if (!g.headless) {
        SDL_UpdateTexture(g.texture, nullptr, g.buffer, kW * sizeof(u32));
        SDL_RenderClear(g.renderer);
        SDL_RenderCopy(g.renderer, g.texture, nullptr, nullptr);
        SDL_RenderPresent(g.renderer);
        // Hard-cap at 60 fps regardless of the display's refresh rate.
        const auto target = std::chrono::microseconds(16667);
        auto elapsed = std::chrono::steady_clock::now() - g.lastPresent;
        if (elapsed < target) std::this_thread::sleep_for(target - elapsed);
        g.lastPresent = std::chrono::steady_clock::now();
    } else {
        // Headless: throttle to roughly real time so background probe workers
        // run and complete as they would against a live window.
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }
    g.frame++;
}

// --- main loop / input ------------------------------------------------------

bool appletMainLoop(void) {
    if (g.frameLimit >= 0 && g.frame >= g.frameLimit) return false;

    g.cur = 0;
    g.stickY = 0;
    if (!g.headless) {
        int wheel = 0;
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) g.quit = true;          // window close only
            if (e.type == SDL_MOUSEWHEEL) wheel += e.wheel.y;
        }
        const u8* k = SDL_GetKeyboardState(nullptr);
        u64 m = 0;
        if (k[SDL_SCANCODE_UP])     m |= HidNpadButton_Up;
        if (k[SDL_SCANCODE_DOWN])   m |= HidNpadButton_Down;
        if (k[SDL_SCANCODE_LEFT])   m |= HidNpadButton_Left;
        if (k[SDL_SCANCODE_RIGHT])  m |= HidNpadButton_Right;
        if (k[SDL_SCANCODE_Z] || k[SDL_SCANCODE_RETURN])  m |= HidNpadButton_A;
        if (k[SDL_SCANCODE_X] || k[SDL_SCANCODE_ESCAPE])  m |= HidNpadButton_B;
        if (k[SDL_SCANCODE_S])      m |= HidNpadButton_X;
        if (k[SDL_SCANCODE_A])      m |= HidNpadButton_Y;
        if (k[SDL_SCANCODE_Q])      m |= HidNpadButton_L;
        if (k[SDL_SCANCODE_W])      m |= HidNpadButton_R;
        if (k[SDL_SCANCODE_1])      m |= HidNpadButton_ZL;
        if (k[SDL_SCANCODE_2])      m |= HidNpadButton_ZR;
        if (k[SDL_SCANCODE_EQUALS]) m |= HidNpadButton_Plus;   // '=' / '+' : exit
        if (k[SDL_SCANCODE_RSHIFT]) m |= HidNpadButton_Minus;
        g.cur = m;

        // Mouse wheel drives the left stick, so ScrollView scrolls with it.
        if (wheel) {
            int s = wheel * 24000;
            g.stickY = s > 32767 ? 32767 : (s < -32767 ? -32767 : s);
        }

        int mx, my;
        u32 mb = SDL_GetMouseState(&mx, &my);
        g.touchOn = (mb & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        g.touchX = mx; g.touchY = my;
    }
    // Scripted input fires for the single matching frame.
    for (int i = 0; i < g.scriptN; i++)
        if (g.script[i].frame == g.frame) g.cur |= g.script[i].btn;
    return !g.quit;
}

u64 armGetSystemTick(void) {
    auto d = std::chrono::steady_clock::now() - g.start;
    return (u64)std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
}

u64 armTicksToNs(u64 tick) { return tick; }   // host ticks are nanoseconds

// --- pad --------------------------------------------------------------------

void padConfigureInput(u32, u32) {}
void padInitializeDefault(PadState* p) { if (p) *p = PadState{}; }

void padUpdate(PadState* p) {
    if (!p) return;
    u64 c = g.cur;
    p->buttons_down = c & ~g.prev;
    p->buttons_up   = ~c & g.prev;
    p->buttons_cur  = c;
    g.prev = c;
    p->sticks[0] = HidAnalogStickState{ 0, g.stickY };
    p->sticks[1] = HidAnalogStickState{};
}

u64 padGetButtons(PadState* p)     { return p ? p->buttons_cur  : 0; }
u64 padGetButtonsDown(PadState* p) { return p ? p->buttons_down : 0; }
u64 padGetButtonsUp(PadState* p)   { return p ? p->buttons_up   : 0; }

HidAnalogStickState padGetStickPos(PadState* p, u32 stick) {
    return (p && stick < 2) ? p->sticks[stick] : HidAnalogStickState{};
}

// --- hid --------------------------------------------------------------------

void hidInitializeTouchScreen(void) {}
void hidInitializeGesture(void) {}

size_t hidGetTouchScreenStates(HidTouchScreenState* states, size_t count) {
    if (!states || count == 0) return 0;
    HidTouchScreenState st{};
    if (g.touchOn) {
        st.count = 1;
        st.touches[0].finger_id = 0;
        st.touches[0].x = (u32)(g.touchX < 0 ? 0 : g.touchX);
        st.touches[0].y = (u32)(g.touchY < 0 ? 0 : g.touchY);
        st.touches[0].diameter_x = st.touches[0].diameter_y = 16;
    }
    states[0] = st;
    return 1;
}
