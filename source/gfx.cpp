#include "nxdisplaylib/gfx.hpp"
#include "nxdisplaylib/font.hpp"
#include <cstring>

namespace nxd {

bool Gfx::init() {
    NWindow* win = nwindowGetDefault();
    if (framebufferCreate(&fb_, win, W, H, PIXEL_FORMAT_RGBA_8888, 2) != 0)
        return false;
    framebufferMakeLinear(&fb_);
    inited_ = true;
    return true;
}

void Gfx::deinit() {
    if (inited_) {
        framebufferClose(&fb_);
        inited_ = false;
    }
}

void Gfx::beginFrame() {
    u32 strideBytes = 0;
    buf_ = (u32*)framebufferBegin(&fb_, &strideBytes);
    stride_ = strideBytes / sizeof(u32);
}

void Gfx::endFrame() {
    framebufferEnd(&fb_);
    buf_ = nullptr;
}

void Gfx::putPixel(int x, int y, u32 color) {
    if (!buf_ || x < 0 || y < 0 || x >= W || y >= H) return;
    buf_[y * stride_ + x] = color;
}

void Gfx::clear(u32 color) {
    fillRect(0, 0, W, H, color);
}

void Gfx::fillRect(int x, int y, int w, int h, u32 color) {
    if (!buf_) return;
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w > W ? W : x + w;
    int y1 = y + h > H ? H : y + h;
    for (int yy = y0; yy < y1; yy++) {
        u32* row = buf_ + yy * stride_;
        for (int xx = x0; xx < x1; xx++) row[xx] = color;
    }
}

void Gfx::hLine(int x, int y, int w, u32 color) { fillRect(x, y, w, 1, color); }
void Gfx::vLine(int x, int y, int h, u32 color) { fillRect(x, y, 1, h, color); }

void Gfx::drawRect(int x, int y, int w, int h, u32 color) {
    if (w <= 0 || h <= 0) return;
    hLine(x, y, w, color);
    hLine(x, y + h - 1, w, color);
    vLine(x, y, h, color);
    vLine(x + w - 1, y, h, color);
}

void Gfx::drawRectThick(int x, int y, int w, int h, int t, u32 color) {
    if (w <= 0 || h <= 0 || t <= 0) return;
    fillRect(x, y, w, t, color);
    fillRect(x, y + h - t, w, t, color);
    fillRect(x, y, t, h, color);
    fillRect(x + w - t, y, t, h, color);
}

void Gfx::drawLine(int x0, int y0, int x1, int y1, u32 color) {
    int dx = x1 - x0, dy = y1 - y0;
    int sx = dx < 0 ? -1 : 1;
    int sy = dy < 0 ? -1 : 1;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    int err = (dx > dy ? dx : -dy) / 2;
    for (;;) {
        putPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = err;
        if (e2 > -dx) { err -= dy; x0 += sx; }
        if (e2 <  dy) { err += dx; y0 += sy; }
    }
}

void Gfx::fillCircle(int cx, int cy, int r, u32 color) {
    if (r <= 0) return;
    for (int dy = -r; dy <= r; dy++) {
        int span = (int)(__builtin_sqrtf((float)(r * r - dy * dy)));
        hLine(cx - span, cy + dy, span * 2 + 1, color);
    }
}

void Gfx::drawCircle(int cx, int cy, int r, u32 color) {
    if (r <= 0) return;
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        putPixel(cx + x, cy + y, color); putPixel(cx + y, cy + x, color);
        putPixel(cx - y, cy + x, color); putPixel(cx - x, cy + y, color);
        putPixel(cx - x, cy - y, color); putPixel(cx - y, cy - x, color);
        putPixel(cx + y, cy - x, color); putPixel(cx + x, cy - y, color);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

void Gfx::drawCross(int cx, int cy, int len, u32 color) {
    hLine(cx - len, cy, len * 2 + 1, color);
    vLine(cx, cy - len, len * 2 + 1, color);
}

void Gfx::blitFull(const u32* src) {
    if (!buf_ || !src) return;
    for (int y = 0; y < H; y++) {
        memcpy(buf_ + y * stride_, src + y * W, W * sizeof(u32));
    }
}

int Gfx::drawChar(int x, int y, int scale, u32 fg, char c) {
    if (scale < 1) scale = 1;
    const u8* g = font::glyph(c);
    for (int row = 0; row < font::GLYPH_H; row++) {
        u8 bits = g[row];
        for (int col = 0; col < font::GLYPH_W; col++) {
            if (bits & (1 << col))
                fillRect(x + col * scale, y + row * scale, scale, scale, fg);
        }
    }
    return font::GLYPH_W * scale;
}

int Gfx::drawText(int x, int y, int scale, u32 fg, const char* s) {
    int cx = x;
    for (; *s; ++s) {
        if (*s == '\n') { cx = x; y += font::GLYPH_H * scale + 2 * scale; continue; }
        cx += drawChar(cx, y, scale, fg, *s);
    }
    return cx - x;
}

int Gfx::textWidth(int scale, const char* s) const {
    int w = 0, line = 0;
    for (; *s; ++s) {
        if (*s == '\n') { if (line > w) w = line; line = 0; }
        else line += font::GLYPH_W * scale;
    }
    return line > w ? line : w;
}

void Gfx::drawTextBg(int x, int y, int scale, u32 fg, u32 bg, const char* s) {
    int pad = 2 * scale;
    int tw = textWidth(scale, s);
    int th = font::GLYPH_H * scale;
    fillRect(x - pad, y - pad, tw + pad * 2, th + pad * 2, bg);
    drawText(x, y, scale, fg, s);
}

void Gfx::drawTextShadow(int x, int y, int scale, u32 fg, u32 shadow, const char* s) {
    drawText(x + scale, y + scale, scale, shadow, s);
    drawText(x, y, scale, fg, s);
}

} // namespace nxd
