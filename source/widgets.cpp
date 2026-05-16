#include "nxdisplaylib/widgets.hpp"
#include "nxdisplaylib/gfx.hpp"

namespace nxd {

// --- ScrollView -------------------------------------------------------------

void ScrollView::reset() {
    scrollPx_   = 0;
    holdFrames_ = 0;
    dragging_   = false;
}

void ScrollView::clamp() {
    maxScrollPx_ = contentH_ - viewportH_;
    if (maxScrollPx_ < 0) maxScrollPx_ = 0;
    if (scrollPx_ > maxScrollPx_) scrollPx_ = maxScrollPx_;
    if (scrollPx_ < 0)            scrollPx_ = 0;
}

void ScrollView::setMetrics(int contentHeight, int viewportHeight) {
    contentH_  = contentHeight;
    viewportH_ = viewportHeight;
    clamp();
}

void ScrollView::handleInput(const Input& in) {
    // D-pad: one step per tap, accelerating while held.
    int dir = 0;
    if (in.held & HidNpadButton_Up)   dir -= 1;
    if (in.held & HidNpadButton_Down) dir += 1;
    if (dir != 0) {
        holdFrames_++;
        int step = (holdFrames_ <= 1) ? 22      // crisp single tap
                 : (holdFrames_ < 16) ? 0       // hold debounce pause
                 : (holdFrames_ < 40) ? 13      // ramp up
                 :                      28;     // sustained fast scroll
        scrollPx_ += dir * step;
    } else {
        holdFrames_ = 0;
    }

    // L / R: page jump.
    int page = viewportH_ > 80 ? viewportH_ - 40 : viewportH_;
    if (in.down & HidNpadButton_L) scrollPx_ -= page;
    if (in.down & HidNpadButton_R) scrollPx_ += page;

    // Left stick: proportional continuous scroll.
    int sy = in.lstick.y;                       // up = positive
    if (sy > 3500 || sy < -3500)
        scrollPx_ -= sy * 42 / 32767;

    // Touch: 1:1 grab-and-drag.
    if (in.touch.count > 0) {
        int ty = in.touch.touches[0].y;
        if (!dragging_) {
            dragging_        = true;
            dragStartTouchY_ = ty;
            dragStartScroll_ = scrollPx_;
        } else {
            scrollPx_ = dragStartScroll_ - (ty - dragStartTouchY_);
        }
    } else {
        dragging_ = false;
    }

    clamp();
}

void ScrollView::drawScrollbar(Gfx& g, int x, int y, int w, int h) const {
    if (contentH_ <= viewportH_ || contentH_ <= 0) return;
    int knobH = h * viewportH_ / contentH_;
    if (knobH < 18) knobH = 18;
    int range = h - knobH;
    int knobY = y + (maxScrollPx_ ? range * scrollPx_ / maxScrollPx_ : 0);
    g.fillRect(x + w - 5, y, 5, h, Gfx::rgb(40, 44, 56));
    g.fillRect(x + w - 5, knobY, 5, knobH, Gfx::rgb(120, 180, 255));
}

// --- ListMenu ---------------------------------------------------------------

void ListMenu::configure(int itemCount, const Layout& layout) {
    itemCount_ = itemCount;
    lay_ = layout;
    if (lay_.perPage < 1) lay_.perPage = 1;
    if (sel_ >= itemCount_) sel_ = itemCount_ > 0 ? itemCount_ - 1 : 0;
    if (sel_ < 0) sel_ = 0;
    page_ = sel_ / lay_.perPage;
}

int ListMenu::pageCount() const {
    if (itemCount_ <= 0) return 1;
    return (itemCount_ + lay_.perPage - 1) / lay_.perPage;
}

int ListMenu::rowAtPoint(int px, int py) const {
    for (int i = 0; i < lay_.perPage; i++) {
        int global = page_ * lay_.perPage + i;
        if (global >= itemCount_) break;
        int ry = lay_.y + i * lay_.rowPitch;
        if (px >= lay_.x && px < lay_.x + lay_.rowW &&
            py >= ry && py < ry + lay_.rowVis)
            return global;
    }
    return -1;
}

int ListMenu::visibleItem(int row, int& x, int& y, int& w, int& h) const {
    int global = page_ * lay_.perPage + row;
    if (row < 0 || row >= lay_.perPage || global >= itemCount_) return -1;
    x = lay_.x;
    y = lay_.y + row * lay_.rowPitch;
    w = lay_.rowW;
    h = lay_.rowVis;
    return global;
}

void ListMenu::handleInput(const Input& in) {
    activated_ = -1;
    if (itemCount_ <= 0) return;
    const int pages = pageCount();

    // D-pad walks every row; L/R flip whole pages.
    if (in.down & HidNpadButton_Down) sel_ = (sel_ + 1) % itemCount_;
    if (in.down & HidNpadButton_Up)   sel_ = (sel_ + itemCount_ - 1) % itemCount_;
    if (in.down & HidNpadButton_R)
        sel_ = ((sel_ / lay_.perPage + 1) % pages) * lay_.perPage;
    if (in.down & HidNpadButton_L)
        sel_ = ((sel_ / lay_.perPage + pages - 1) % pages) * lay_.perPage;
    if (sel_ >= itemCount_) sel_ = itemCount_ - 1;
    page_ = sel_ / lay_.perPage;

    // Touch: a row activates only if the press AND the release land inside it.
    bool touching = in.touch.count > 0;
    if (touching) {
        lastTouchX_ = in.touch.touches[0].x;
        lastTouchY_ = in.touch.touches[0].y;
    }
    if (touching && !wasTouching_) {
        touchRow_ = rowAtPoint(lastTouchX_, lastTouchY_);
        if (touchRow_ >= 0) { sel_ = touchRow_; page_ = sel_ / lay_.perPage; }
    }
    bool released = !touching && wasTouching_;
    wasTouching_ = touching;

    if (in.down & HidNpadButton_A)
        activated_ = sel_;
    if (released) {
        int r = rowAtPoint(lastTouchX_, lastTouchY_);
        if (r >= 0 && r == touchRow_) activated_ = r;
        touchRow_ = -1;
    }
}

} // namespace nxd
