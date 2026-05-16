// Host (PC) mock of the libnx surface that the nxdisplaylib framework touches.
//
// It declares just enough of <switch.h> for gfx / font / input / view / runner
// / widgets to compile and run unmodified on a desktop. An application's host
// build includes this from its own <switch.h> and adds its extra mocks.
//
// Struct layouts mirror libnx by field name; they need not be binary
// compatible since nothing here talks to real hardware.
#pragma once
#include <stdint.h>
#include <stddef.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef u32 Result;
#define R_SUCCEEDED(res) ((res) == 0)
#define R_FAILED(res)    ((res) != 0)

#ifndef BIT
#define BIT(n) (1U << (n))
#endif

// --- framebuffer / window ---------------------------------------------------
#define PIXEL_FORMAT_RGBA_8888 1
#define RGBA8(r, g, b, a) (((r) & 0xff) | (((g) & 0xff) << 8) | \
                           (((b) & 0xff) << 16) | (((a) & 0xff) << 24))
#define RGBA8_MAXALPHA(r, g, b) RGBA8(r, g, b, 0xff)

typedef struct { int unused; } NWindow;
typedef struct { int unused; } Framebuffer;

NWindow* nwindowGetDefault(void);
Result   framebufferCreate(Framebuffer* fb, NWindow* win, u32 width, u32 height,
                           u32 format, u32 num_fbs);
Result   framebufferMakeLinear(Framebuffer* fb);
void     framebufferClose(Framebuffer* fb);
void*    framebufferBegin(Framebuffer* fb, u32* out_stride);
void     framebufferEnd(Framebuffer* fb);

// --- buttons ----------------------------------------------------------------
enum {
    HidNpadButton_A          = BIT(0),
    HidNpadButton_B          = BIT(1),
    HidNpadButton_X          = BIT(2),
    HidNpadButton_Y          = BIT(3),
    HidNpadButton_StickL     = BIT(4),
    HidNpadButton_StickR     = BIT(5),
    HidNpadButton_L          = BIT(6),
    HidNpadButton_R          = BIT(7),
    HidNpadButton_ZL         = BIT(8),
    HidNpadButton_ZR         = BIT(9),
    HidNpadButton_Plus       = BIT(10),
    HidNpadButton_Minus      = BIT(11),
    HidNpadButton_Left       = BIT(12),
    HidNpadButton_Up         = BIT(13),
    HidNpadButton_Right      = BIT(14),
    HidNpadButton_Down       = BIT(15),
};

#define HidNpadStyleSet_NpadStandard 1u

// --- touch ------------------------------------------------------------------
enum {
    HidTouchAttribute_Start = BIT(0),
    HidTouchAttribute_End   = BIT(1),
};

typedef struct HidTouchState {
    u64 delta_time;
    u32 attributes;
    u32 finger_id;
    u32 x, y;
    u32 diameter_x, diameter_y;
    u32 rotation_angle;
    u32 reserved;
} HidTouchState;

typedef struct HidTouchScreenState {
    u64 sampling_number;
    s32 count;
    u32 reserved;
    HidTouchState touches[16];
} HidTouchScreenState;

// --- analog stick -----------------------------------------------------------
typedef struct HidAnalogStickState {
    s32 x, y;
} HidAnalogStickState;

// --- pad --------------------------------------------------------------------
typedef struct PadState {
    u64 buttons_cur, buttons_down, buttons_up;
    HidAnalogStickState sticks[2];
} PadState;

void   padConfigureInput(u32 max_players, u32 style_set);
void   padInitializeDefault(PadState* pad);
void   padUpdate(PadState* pad);
u64    padGetButtons(PadState* pad);
u64    padGetButtonsDown(PadState* pad);
u64    padGetButtonsUp(PadState* pad);
HidAnalogStickState padGetStickPos(PadState* pad, u32 stick);

// --- hid --------------------------------------------------------------------
void   hidInitializeTouchScreen(void);
void   hidInitializeGesture(void);
size_t hidGetTouchScreenStates(HidTouchScreenState* states, size_t count);

// --- applet / timing --------------------------------------------------------
bool appletMainLoop(void);
u64  armGetSystemTick(void);
u64  armTicksToNs(u64 tick);   // host mock: ticks are already nanoseconds
