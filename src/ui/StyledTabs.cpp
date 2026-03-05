#include "ui/StyledTabs.h"

#include <FL/Fl.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>

namespace verdad {

StyledTabs::StyledTabs(int X, int Y, int W, int H, const char* L)
    : Fl_Tabs(X, Y, W, H, L) {}

void StyledTabs::draw() {
    if (fillParentBackground_) {
        fl_push_clip(x(), y(), w(), h());
        Fl_Color bg = parent() ? parent()->color() : FL_BACKGROUND_COLOR;
        fl_color(bg);
        fl_rectf(x(), y(), w(), h());
        fl_pop_clip();
    }

    applySelectedTabFonts();
    applyCloseFlags();
    Fl_Tabs::draw();
}

void StyledTabs::resize(int X, int Y, int W, int H) {
    Fl_Tabs::resize(X, Y, W, H);
    damage(FL_DAMAGE_ALL);
    redraw();
}

int StyledTabs::handle(int event) {
    // Intercept close button clicks to route through our closeCb_
    // instead of FLTK calling the child widget's callback.
    if (event == FL_RELEASE && showClose_ && children() > 1) {
        Fl_Widget* o = which(Fl::event_x(), Fl::event_y());
        if (o && o == value() && (o->when() & FL_WHEN_CLOSED)) {
            if (hit_close(o, Fl::event_x(), Fl::event_y())) {
                if (closeCb_) closeCb_(o);
                return 1;
            }
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

void StyledTabs::applyCloseFlags() {
    if (!showClose_ || children() <= 1) {
        // Clear FL_WHEN_CLOSED on all children.
        for (int i = 0; i < children(); ++i) {
            Fl_Widget* child = this->child(i);
            if (child)
                child->when(child->when() & ~FL_WHEN_CLOSED);
        }
        return;
    }

    // Set FL_WHEN_CLOSED only on the selected child.
    Fl_Widget* selected = value();
    for (int i = 0; i < children(); ++i) {
        Fl_Widget* child = this->child(i);
        if (!child) continue;
        if (child == selected)
            child->when(child->when() | FL_WHEN_CLOSED);
        else
            child->when(child->when() & ~FL_WHEN_CLOSED);
    }
}

void StyledTabs::updateCloseButtons() {
    showClose_ = children() > 1;
    redraw();
}

} // namespace verdad
