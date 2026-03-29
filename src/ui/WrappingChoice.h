#ifndef VERDAD_WRAPPING_CHOICE_H
#define VERDAD_WRAPPING_CHOICE_H

#include "ui/ChoicePopupUtils.h"

#include <FL/Fl_Choice.H>

#include <string>
#include <vector>

class Fl_Double_Window;
class Fl_Hold_Browser;

namespace verdad {

class WrappingChoice : public Fl_Choice, public choice_popup::PopupOwner {
public:
    WrappingChoice(int X, int Y, int W, int H, const char* label = nullptr);
    ~WrappingChoice() override;

    int handle(int event) override;
    void resize(int X, int Y, int W, int H) override;

    Fl_Widget* popupAnchorWidget() const override;
    void showOwnedPopup() override;
    void hideOwnedPopup() override;
    bool popupIsVisible() const override;
    void takeOwnedFocus() override;

private:
    friend class WrappingChoicePopupWindow;
    friend class WrappingChoiceColumnBrowser;

    struct PopupEntry {
        int menuIndex = -1;
        std::string label;
        bool active = true;
    };

    std::vector<PopupEntry> popupEntries_;
    std::vector<std::vector<int>> popupColumnEntries_;
    std::vector<Fl_Hold_Browser*> popupBrowsers_;
    Fl_Double_Window* popupWindow_ = nullptr;

    void destroyPopup();
    bool supportsCustomPopup() const;
    bool buildPopup();
    bool showPopup();
    void hidePopup();
    bool popupVisible() const;
    bool popupContainsRootPoint(int rootX, int rootY) const;
    int currentPopupColumn() const;
    int selectedLineForColumn(int columnIndex) const;
    void focusColumn(int columnIndex, int preferredLine = 0);
    void moveFocus(int deltaColumns);
    void selectBrowserLine(Fl_Hold_Browser* browser, int line);
    int columnIndexForBrowser(const Fl_Hold_Browser* browser) const;

    static void onPopupPicked(Fl_Widget* widget, void* data);
};

} // namespace verdad

#endif // VERDAD_WRAPPING_CHOICE_H
