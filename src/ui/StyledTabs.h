#ifndef VERDAD_STYLED_TABS_H
#define VERDAD_STYLED_TABS_H

#include <FL/Fl_Tabs.H>
#include <functional>

namespace verdad {

/// Fl_Tabs helper with stable redraw, selected-tab bold labels, and
/// per-tab close buttons via FLTK's native FL_WHEN_CLOSED mechanism.
class StyledTabs : public Fl_Tabs {
public:
    StyledTabs(int X, int Y, int W, int H, const char* L = nullptr);

    void setFillParentBackground(bool fill) { fillParentBackground_ = fill; }

    /// Set the callback invoked when a tab's close button is clicked.
    /// The argument is the Fl_Group* of the closed tab.
    void setCloseCallback(std::function<void(Fl_Widget*)> cb) { closeCb_ = std::move(cb); }

    /// Update close-button visibility based on the number of children.
    /// Call after adding or removing tabs.
    void updateCloseButtons();

    void draw() override;
    void resize(int X, int Y, int W, int H) override;

private:
    static Fl_Font regularTabFont(Fl_Font font);
    static Fl_Font boldTabFont(Fl_Font font);
    void applySelectedTabFonts();

    /// Callback installed on each child to handle FL_REASON_CLOSED.
    static void onChildCallback(Fl_Widget* w, void* data);

    bool fillParentBackground_ = true;
    std::function<void(Fl_Widget*)> closeCb_;
};

} // namespace verdad

#endif // VERDAD_STYLED_TABS_H
