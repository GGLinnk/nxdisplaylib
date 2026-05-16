# Host (PC) build of the nxdisplaylib view framework.
#
#   make            build build/smoke
#   make run        build and run the framework smoke test (SDL window)
#   make headless   run 180 frames with no window, dump build/smoke.ppm
#   make clean      remove build/
#
# The framework compiles unmodified: -I. makes <switch.h> resolve to the mock
# in this directory; nxd_host_libnx.h + host_backend.cpp stand in for libnx.

CXX      ?= g++
LIB      := ..
CXXFLAGS := -std=gnu++17 -O2 -Wall -Wextra -I. -I$(LIB)/include $(shell pkg-config --cflags sdl2)
LDLIBS   := $(shell pkg-config --libs sdl2)

LIBSRC := $(LIB)/source/gfx.cpp $(LIB)/source/font.cpp \
          $(LIB)/source/runner.cpp $(LIB)/source/widgets.cpp
SRC    := $(LIBSRC) host_backend.cpp smoke.cpp
DEPS   := nxd_host_libnx.h switch.h

BIN := build/smoke

$(BIN): $(SRC) $(DEPS)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(BIN)
	./$(BIN)

headless: $(BIN)
	NXD_HOST_FRAMES=180 NXD_HOST_DUMP=build/smoke.ppm ./$(BIN)

clean:
	rm -rf build

.PHONY: run headless clean
