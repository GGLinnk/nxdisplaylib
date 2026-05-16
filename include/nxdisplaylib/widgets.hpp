#pragma once
#include "nxdisplaylib/input.hpp"

namespace nxd {

class Gfx;

// --- ScrollView -------------------------------------------------------------
// A vertical scroll viewport. Each frame the owner sets the content/viewport
// metrics, feeds input, then draws its content shifted up by offset(). Accepts
// D-pad (accelerating while held), L/R paging, left-stick and touch-drag.
class ScrollView {
public:
    void reset();
    // Call each frame before handleInput() / drawing.
    void setMetrics(int contentHeight, int viewportHeight);
    void handleInput(const Input& in);
    int  offset() const { return scrollPx_; }
    // Draw a scrollbar down the right edge of the rectangle (x,y,w,h).
    void drawScrollbar(Gfx& g, int x, int y, int w, int h) const;

private:
    void clamp();
    int  scrollPx_ = 0, maxScrollPx_ = 0;
    int  viewportH_ = 0, contentH_ = 0;
    int  holdFrames_ = 0;
    bool dragging_ = false;
    int  dragStartTouchY_ = 0, dragStartScroll_ = 0;
};

// --- ListMenu ---------------------------------------------------------------
// A vertical list of selectable rows with D-pad navigation, L/R paging and
// touch hit-testing. The widget owns selection / paging / input; the caller
// draws each row. A row activates only when A is pressed, or a touch both
// presses and releases inside the same row.
class ListMenu {
public:
    struct Layout {
        int x = 0, y = 0;     // top-left of the row list
        int rowPitch = 0;     // vertical distance between row origins
        int rowW = 0;         // row width
        int rowVis = 0;       // drawn / touchable row height
        int perPage = 1;      // rows shown per page
    };

    void configure(int itemCount, const Layout& layout);
    void handleInput(const Input& in);

    int  selected()  const { return sel_; }
    int  page()      const { return page_; }
    int  pageCount() const;
    // Global item index activated this frame (A or completed tap), else -1.
    int  activated() const { return activated_; }

    // Maps a visible row (0..perPage-1) to its global item index and on-screen
    // rectangle. Returns -1 when the slot is past the end of the list.
    int  visibleItem(int row, int& x, int& y, int& w, int& h) const;

private:
    int rowAtPoint(int px, int py) const;

    Layout lay_{};
    int  itemCount_ = 0;
    int  sel_ = 0, page_ = 0;
    int  activated_ = -1;
    bool wasTouching_ = false;
    int  touchRow_ = -1, lastTouchX_ = 0, lastTouchY_ = 0;
};

} // namespace nxd
