#include "ui/StyledTabs.h"

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>

namespace verdad {

StyledTabs::StyledTabs(int X, int Y, int W, int H, const char* L)
    : Fl_Tabs(X, Y, W, H, L) {}

void StyledTabs::draw() {
    closeBtnValid_ = false;

    if (fillParentBackground_) {
        fl_push_clip(x(), y(), w(), h());
        Fl_Color bg = parent() ? parent()->color() : FL_BACKGROUND_COLOR;
        fl_color(bg);
        fl_rectf(x(), y(), w(), h());
        fl_pop_clip();
    }

    applySelectedTabFonts();
    Fl_Tabs::draw();
}

void StyledTabs::resize(int X, int Y, int W, int H) {
    Fl_Tabs::resize(X, Y, W, H);
    closeBtnValid_ = false;
    damage(FL_DAMAGE_ALL);
    redraw();
}

int StyledTabs::tab_positions() {
    int ret = Fl_Tabs::tab_positions();
    if (!showClose_ || children() <= 1) return ret;

    Fl_Widget* sel = value();
    int n = children();
    for (int i = 0; i < n; i++) {
        if (child(i) == sel) {
            int extra = kCloseBtnSize + kCloseBtnPad * 2;
            tab_width[i] += extra;
            // Shift all subsequent tab positions to accommodate the wider tab.
            for (int j = i + 1; j <= n; j++)
                tab_pos[j] += extra;
            break;
        }
    }
    return ret;
}

void StyledTabs::draw_tab(int x1, int x2, int W, int H,
                           Fl_Widget* o, int flags, int sel) {
    // Let FLTK draw the tab shape and label. The selected tab's W was
    // increased by tab_positions(), so the label is centered with room
    // left on the right for the close button.
    Fl_Tabs::draw_tab(x1, x2, W, H, o, flags, sel);

    if (sel && showClose_ && children() > 1) {
        int tabH = (H > 0) ? H : -H;
        int tabY = (H > 0) ? y() : (y() + h() + H);

        closeBtnX_ = x1 + W - kCloseBtnSize - kCloseBtnPad;
        closeBtnY_ = tabY + (tabH - kCloseBtnSize) / 2 + 1;
        closeBtnValid_ = true;

        // Button background — subtle highlight on hover.
        Fl_Color btnBg = closeHovered_
            ? fl_color_average(FL_BACKGROUND_COLOR, FL_FOREGROUND_COLOR, 0.82f)
            : selection_color();  // match selected-tab background
        fl_rectf(closeBtnX_, closeBtnY_, kCloseBtnSize, kCloseBtnSize, btnBg);

        if (closeHovered_) {
            fl_color(fl_color_average(FL_BACKGROUND_COLOR, FL_FOREGROUND_COLOR, 0.65f));
            fl_rect(closeBtnX_, closeBtnY_, kCloseBtnSize, kCloseBtnSize);
        }

        // Draw the X glyph.
        fl_color(closeHovered_ ? FL_FOREGROUND_COLOR : fl_inactive(FL_FOREGROUND_COLOR));
        int p = 4;  // padding inside the button
        int bx = closeBtnX_, by = closeBtnY_;
        fl_line(bx + p, by + p, bx + kCloseBtnSize - p - 1, by + kCloseBtnSize - p - 1);
        fl_line(bx + p, by + kCloseBtnSize - p - 1, bx + kCloseBtnSize - p - 1, by + p);
    }
}

int StyledTabs::handle(int event) {
    if (showClose_ && children() > 1) {
        switch (event) {
        case FL_PUSH:
            if (closeBtnValid_) {
                int ex = Fl::event_x(), ey = Fl::event_y();
                if (ex >= closeBtnX_ && ex < closeBtnX_ + kCloseBtnSize &&
                    ey >= closeBtnY_ && ey < closeBtnY_ + kCloseBtnSize) {
                    Fl_Widget* sel = value();
                    if (sel && closeCb_) closeCb_(sel);
                    return 1;
                }
            }
            break;
        case FL_MOVE:
        case FL_ENTER: {
            bool hover = false;
            if (closeBtnValid_) {
                int ex = Fl::event_x(), ey = Fl::event_y();
                hover = (ex >= closeBtnX_ && ex < closeBtnX_ + kCloseBtnSize &&
                         ey >= closeBtnY_ && ey < closeBtnY_ + kCloseBtnSize);
            }
            if (hover != closeHovered_) {
                closeHovered_ = hover;
                redraw_tabs();
            }
            break;
        }
        case FL_LEAVE:
            if (closeHovered_) {
                closeHovered_ = false;
                redraw_tabs();
            }
            break;
        }
    }
    return Fl_Tabs::handle(event);
}

void StyledTabs::applySelectedTabFonts() {
    Fl_Widget* selected = value();
    for (int i = 0; i < children(); ++i) {
        Fl_Widget* child = this->child(i);
        if (!child) continue;
        child->labelfont(child == selected ? boldLabelFont_ : baseLabelFont_);
    }
}

void StyledTabs::updateCloseButtons() {
    showClose_ = children() > 1;
    closeBtnValid_ = false;
    closeHovered_ = false;
    redraw();
}

} // namespace verdad
