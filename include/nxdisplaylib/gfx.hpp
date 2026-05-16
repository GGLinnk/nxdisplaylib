#pragma once
#include <switch.h>

namespace nxd {

// Software renderer on top of the libnx Framebuffer API.
// Owns the default window's double-buffered RGBA8888 framebuffer and exposes
// clipped drawing primitives plus bitmap text rendering.
class Gfx {
public:
    static constexpr int W = 1280;
    static constexpr int H = 720;

    bool init();
    void deinit();

    // Frame lifecycle: beginFrame() acquires the back buffer, endFrame() presents it.
    void beginFrame();
    void endFrame();

    // Pack an opaque colour. Order matches PIXEL_FORMAT_RGBA_8888.
    static inline u32 rgb(u8 r, u8 g, u8 b) { return RGBA8_MAXALPHA(r, g, b); }

    // --- primitives (all clipped to the framebuffer) ---
    void clear(u32 color);
    void putPixel(int x, int y, u32 color);
    void fillRect(int x, int y, int w, int h, u32 color);
    void drawRect(int x, int y, int w, int h, u32 color);          // 1px outline
    void drawRectThick(int x, int y, int w, int h, int t, u32 color);
    void hLine(int x, int y, int w, u32 color);
    void vLine(int x, int y, int h, u32 color);
    void drawLine(int x0, int y0, int x1, int y1, u32 color);
    void fillCircle(int cx, int cy, int r, u32 color);
    void drawCircle(int cx, int cy, int r, u32 color);
    void drawCross(int cx, int cy, int len, u32 color);

    // Copy a full-screen WxH source buffer straight into the back buffer.
    void blitFull(const u32* src);

    // --- text (8x8 glyphs, integer scaled) ---
    int  charW(int scale) const { return 8 * scale; }
    int  charH(int scale) const { return 8 * scale; }
    int  textWidth(int scale, const char* s) const;
    int  drawChar(int x, int y, int scale, u32 fg, char c);   // returns advance
    int  drawText(int x, int y, int scale, u32 fg, const char* s);
    // Text with a padded solid background box (legible over any pattern).
    void drawTextBg(int x, int y, int scale, u32 fg, u32 bg, const char* s);
    // Text with a 1px drop shadow — legible over any background, no box.
    void drawTextShadow(int x, int y, int scale, u32 fg, u32 shadow, const char* s);

private:
    Framebuffer fb_{};
    u32* buf_ = nullptr;   // current back buffer
    u32  stride_ = 0;      // stride in pixels
    bool inited_ = false;
};

} // namespace nxd
