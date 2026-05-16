#pragma once
#include <switch.h>

// Self-contained 8x8 bitmap font (public-domain "font8x8" set) covering the
// printable ASCII range 0x20..0x7F. No RomFS / asset files required.
namespace nxd {
namespace font {

constexpr int GLYPH_W = 8;
constexpr int GLYPH_H = 8;

// Raw glyph data: one byte per row, bit 0 (LSB) = leftmost pixel.
extern const u8 glyphs[96][8];

// Returns the 8-byte glyph rows for a character, falling back to '?'.
const u8* glyph(char c);

} // namespace font
} // namespace nxd
