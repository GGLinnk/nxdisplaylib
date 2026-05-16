# nxdisplaylib

A small display + view-framework library for Nintendo Switch homebrew, built
on the libnx Framebuffer API. Short namespace: **`nxd`**.

It provides both the low-level rendering substrate and the application
scaffolding a homebrew tool's screens are built on.

## Components

```
include/nxdisplaylib/
  gfx.hpp      nxd::Gfx       framebuffer wrapper, clipped 2D primitives,
                              scaled bitmap text
  font.hpp     nxd::font      embedded 8x8 ASCII bitmap font (no assets)
  input.hpp    nxd::Input     per-frame input snapshot (buttons/touch/sticks)
  view.hpp     nxd::View      a navigable screen interface
  runner.hpp   nxd::Runner    the main-loop host: input, navigation, chrome
  widgets.hpp  nxd::ScrollView   pixel-smooth scroll viewport
               nxd::ListMenu     selectable, paged, touch-hit-tested list
source/
  gfx.cpp  font.cpp  runner.cpp  widgets.cpp
```

A program implements its screens as `nxd::View` subclasses and hands them to a
`nxd::Runner`, which owns the renderer, polls input, drives the active view,
handles navigation (ZL/ZR cycle, B-home, +-exit) and draws the chrome.

## Using it

Add nxdisplaylib as a git submodule:

```sh
git submodule add https://github.com/GGLinnk/nxdisplaylib.git libs/nxdisplaylib
```

and point the devkitPro Makefile at the two directories:

```make
SOURCES  := source libs/nxdisplaylib/source
INCLUDES := source libs/nxdisplaylib/include
```

Sources then include the namespaced headers and may pull the whole library
namespace in unqualified:

```c
#include "nxdisplaylib/runner.hpp"
using namespace nxd;
```

Clone with `--recursive` (or run `git submodule update --init`) so the library
is present; CI checks out with `submodules: recursive`.

## Host (PC) build

`host/` lets the framework build and run on a desktop, no Switch or emulator.
A mock `<switch.h>` (`nxd_host_libnx.h`) plus an SDL2 backend (`host_backend.cpp`)
stand in for libnx, so the framework compiles unmodified.

```sh
cd host
make run        # framework smoke test in an SDL2 window
make headless   # 180 frames, no window, dump build/smoke.ppm
```

Needs `g++` and `libsdl2-dev`. Keyboard: arrows = D-pad, Z/X = A/B, A/S = Y/X,
Q/W = L/R, 1/2 = ZL/ZR, Enter/RShift = +/-, Esc quits; mouse = touch.

