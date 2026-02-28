#ifndef VERDAD_STYLED_TABS_H
#define VERDAD_STYLED_TABS_H

#include <FL/Fl_Tabs.H>

namespace verdad {

/// Fl_Tabs helper with stable redraw and selected-tab bold labels.
class StyledTabs : public Fl_Tabs {
public:
    StyledTabs(int X, int Y, int W, int H, const char* L = nullptr);

    void setFillParentBackground(bool fill) { fillParentBackground_ = fill; }

    void draw() override;
    void resize(int X, int Y, int W, int H) override;

private:
    static Fl_Font regularTabFont(Fl_Font font);
    static Fl_Font boldTabFont(Fl_Font font);
    void applySelectedTabFonts();

    bool fillParentBackground_ = true;
};

} // namespace verdad

#endif // VERDAD_STYLED_TABS_H
