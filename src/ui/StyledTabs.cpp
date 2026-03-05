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
    Fl_Tabs::draw();
}

void StyledTabs::resize(int X, int Y, int W, int H) {
    Fl_Tabs::resize(X, Y, W, H);
    damage(FL_DAMAGE_ALL);
    redraw();
}

Fl_Font StyledTabs::regularTabFont(Fl_Font font) {
    // Built-in fonts (0-15): explicit mapping
    switch (font) {
    case FL_HELVETICA_BOLD: return FL_HELVETICA;
    case FL_HELVETICA_BOLD_ITALIC: return FL_HELVETICA_ITALIC;
    case FL_COURIER_BOLD: return FL_COURIER;
    case FL_COURIER_BOLD_ITALIC: return FL_COURIER_ITALIC;
    case FL_TIMES_BOLD: return FL_TIMES;
    case FL_TIMES_BOLD_ITALIC: return FL_TIMES_ITALIC;
    default: break;
    }
    // System fonts (≥16): FLTK groups families in sets of 4 (regular, bold, italic, bold-italic).
    // Clear the bold bit to get the regular variant.
    if (font >= 16)
        return static_cast<Fl_Font>(font & ~1);
    return font;
}

Fl_Font StyledTabs::boldTabFont(Fl_Font font) {
    // Built-in fonts (0-15): explicit mapping
    switch (font) {
    case FL_HELVETICA:
    case FL_HELVETICA_BOLD:
    case FL_HELVETICA_ITALIC:
    case FL_HELVETICA_BOLD_ITALIC:
        return FL_HELVETICA_BOLD;
    case FL_COURIER:
    case FL_COURIER_BOLD:
    case FL_COURIER_ITALIC:
    case FL_COURIER_BOLD_ITALIC:
        return FL_COURIER_BOLD;
    case FL_TIMES:
    case FL_TIMES_BOLD:
    case FL_TIMES_ITALIC:
    case FL_TIMES_BOLD_ITALIC:
        return FL_TIMES_BOLD;
    default: break;
    }
    // System fonts (≥16): set the bold bit (bit 0).
    if (font >= 16)
        return static_cast<Fl_Font>(font | 1);
    return font;
}

void StyledTabs::applySelectedTabFonts() {
    Fl_Widget* selected = value();
    for (int i = 0; i < children(); ++i) {
        Fl_Widget* child = this->child(i);
        if (!child) continue;

        Fl_Font regular = regularTabFont(child->labelfont());
        child->labelfont(child == selected ? boldTabFont(regular) : regular);
    }
}

void StyledTabs::updateCloseButtons() {
    bool showClose = children() > 1;
    for (int i = 0; i < children(); ++i) {
        Fl_Widget* child = this->child(i);
        if (!child) continue;
        if (showClose) {
            child->when(child->when() | FL_WHEN_CLOSED);
            child->callback(onChildCallback, this);
        } else {
            child->when(child->when() & ~FL_WHEN_CLOSED);
        }
    }
    redraw();
}

void StyledTabs::onChildCallback(Fl_Widget* w, void* data) {
    if (Fl::callback_reason() != FL_REASON_CLOSED) return;
    auto* tabs = static_cast<StyledTabs*>(data);
    if (tabs && tabs->closeCb_)
        tabs->closeCb_(w);
}

} // namespace verdad
