#ifndef VERDAD_STYLED_TABS_H
#define VERDAD_STYLED_TABS_H

#include <FL/Fl_Tabs.H>
#include <functional>

namespace verdad {

/// Fl_Tabs subclass with bold selected-tab labels, and a custom close button
/// drawn on the right side of the active tab only.
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

protected:
    void draw_tab(int x1, int x2, int W, int H, Fl_Widget* o, int flags, int sel) override;
    int tab_positions() override;

private:
    static constexpr int kCloseBtnSize = 16;
    static constexpr int kCloseBtnPad = 4;

    void applySelectedTabFonts();

    bool fillParentBackground_ = true;
    Fl_Font baseLabelFont_ = FL_HELVETICA;
    Fl_Font boldLabelFont_ = FL_HELVETICA_BOLD;
    bool showClose_ = false;
    bool closeHovered_ = false;
    /// Close button screen rect, set during draw_tab for the selected tab.
    int closeBtnX_ = 0, closeBtnY_ = 0;
    bool closeBtnValid_ = false;

    std::function<void(Fl_Widget*)> closeCb_;
};

} // namespace verdad

#endif // VERDAD_STYLED_TABS_H
