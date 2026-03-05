#ifndef VERDAD_STYLED_TABS_H
#define VERDAD_STYLED_TABS_H

#include <FL/Fl_Tabs.H>
#include <functional>

namespace verdad {

/// Fl_Tabs subclass with bold selected-tab labels, and a close button
/// on the active tab using FLTK's native FL_WHEN_CLOSED mechanism.
class StyledTabs : public Fl_Tabs {
public:
    StyledTabs(int X, int Y, int W, int H, const char* L = nullptr);

    void setFillParentBackground(bool fill) { fillParentBackground_ = fill; }

    /// Set the regular and bold fonts used for tab labels.
    void setLabelFonts(Fl_Font regular, Fl_Font bold) {
        baseLabelFont_ = regular;
        boldLabelFont_ = bold;
    }

    /// Set the callback invoked when the close button is clicked.
    /// The argument is the Fl_Group* of the tab being closed.
    void setCloseCallback(std::function<void(Fl_Widget*)> cb) { closeCb_ = std::move(cb); }

    /// Update close-button visibility based on the number of children.
    void updateCloseButtons();

    void draw() override;
    void resize(int X, int Y, int W, int H) override;
    int handle(int event) override;

private:
    void applySelectedTabFonts();
    void applyCloseFlags();

    bool fillParentBackground_ = true;
    Fl_Font baseLabelFont_ = FL_HELVETICA;
    Fl_Font boldLabelFont_ = FL_HELVETICA_BOLD;
    bool showClose_ = false;

    std::function<void(Fl_Widget*)> closeCb_;
};

} // namespace verdad

#endif // VERDAD_STYLED_TABS_H
