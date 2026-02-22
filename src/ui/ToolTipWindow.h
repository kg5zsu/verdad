#ifndef VERDAD_TOOLTIP_WINDOW_H
#define VERDAD_TOOLTIP_WINDOW_H

#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Box.H>
#include <string>

namespace verdad {

class HtmlWidget;

/// Floating tooltip window for displaying word info (Strong's, morphology).
/// Similar to BibleTime's MAG viewer or Xiphos's info window.
class ToolTipWindow : public Fl_Menu_Window {
public:
    ToolTipWindow();
    ~ToolTipWindow() override;

    /// Show the tooltip at the given screen position with HTML content
    void showAt(int screenX, int screenY, const std::string& html);

    /// Hide the tooltip
    void hideTooltip();

    /// Check if tooltip is currently shown
    bool isShown() const { return shown_; }

protected:
    void draw() override;
    int handle(int event) override;

private:
    Fl_Box* label_;
    bool shown_ = false;
    std::string currentText_;
};

} // namespace verdad

#endif // VERDAD_TOOLTIP_WINDOW_H
