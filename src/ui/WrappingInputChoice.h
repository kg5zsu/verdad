#ifndef VERDAD_WRAPPING_INPUT_CHOICE_H
#define VERDAD_WRAPPING_INPUT_CHOICE_H

#include "ui/ChoicePopupUtils.h"

#include <FL/Fl_Input_Choice.H>

#include <string>
#include <vector>

class Fl_Double_Window;
class Fl_Hold_Browser;

namespace verdad {

class WrappingInputChoice : public Fl_Input_Choice, public choice_popup::PopupOwner {
public:
    WrappingInputChoice(int X, int Y, int W, int H, const char* label = nullptr);
    ~WrappingInputChoice() override;

    int handle(int event) override;
    void resize(int X, int Y, int W, int H) override;

    void setPopupInitialIndex(int index);
    bool consumeMenuSelectionCallback();

    Fl_Widget* popupAnchorWidget() const override;
    void showOwnedPopup() override;
    void hideOwnedPopup() override;
    bool popupIsVisible() const override;
    void takeOwnedFocus() override;

private:
    friend class WrappingInputChoicePopupWindow;
    friend class WrappingInputChoiceColumnBrowser;

    struct PopupEntry {
        int menuIndex = -1;
        std::string label;
        bool active = true;
    };

    int popupInitialIndex_ = -1;
    bool menuSelectionPending_ = false;
    std::vector<PopupEntry> popupEntries_;
    std::vector<std::vector<int>> popupColumnEntries_;
    std::vector<Fl_Hold_Browser*> popupBrowsers_;
    Fl_Double_Window* popupWindow_ = nullptr;

    void destroyPopup();
    bool eventInMenuButton() const;
    bool supportsCustomPopup() const;
    bool buildPopup();
    bool showPopup();
    void hidePopup();
    bool popupVisible() const;
    bool popupContainsRootPoint(int rootX, int rootY) const;
    int selectedMenuIndex() const;
    int currentPopupColumn() const;
    int selectedLineForColumn(int columnIndex) const;
    void focusColumn(int columnIndex, int preferredLine = 0);
    void moveFocus(int deltaColumns);
    void selectBrowserLine(Fl_Hold_Browser* browser, int line);
    int columnIndexForBrowser(const Fl_Hold_Browser* browser) const;

    static void onPopupPicked(Fl_Widget* widget, void* data);
};

} // namespace verdad

#endif // VERDAD_WRAPPING_INPUT_CHOICE_H
