#ifndef VERDAD_CHOICE_POPUP_UTILS_H
#define VERDAD_CHOICE_POPUP_UTILS_H

#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Window.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <string>
#include <vector>
#include <utility>

namespace verdad::choice_popup {

class PopupOwner {
public:
    virtual ~PopupOwner() = default;

    virtual Fl_Widget* popupAnchorWidget() const = 0;
    virtual void showOwnedPopup() = 0;
    virtual void hideOwnedPopup() = 0;
    virtual bool popupIsVisible() const = 0;
    virtual void takeOwnedFocus() = 0;
};

inline constexpr int kPopupDefaultMinColumnW = 90;
inline constexpr int kPopupDefaultMaxColumnW = 450;
inline constexpr int kPopupDefaultMinCappedColumnW = 240;

inline std::vector<PopupOwner*>& popupOwners() {
    static std::vector<PopupOwner*> owners;
    return owners;
}

inline PopupOwner*& activePopupOwner() {
    static PopupOwner* owner = nullptr;
    return owner;
}

inline void registerPopupOwner(PopupOwner* owner) {
    if (!owner) return;
    popupOwners().push_back(owner);
}

inline void unregisterPopupOwner(PopupOwner* owner) {
    if (!owner) return;

    auto& owners = popupOwners();
    owners.erase(std::remove(owners.begin(), owners.end(), owner), owners.end());
    if (activePopupOwner() == owner) {
        activePopupOwner() = nullptr;
    }
}

inline std::pair<int, int> rootPosition(const Fl_Widget* widget) {
    if (!widget) return {0, 0};

    int rootX = 0;
    int rootY = 0;
    Fl_Window* top = widget->top_window_offset(rootX, rootY);
    if (top) {
        rootX += top->x_root();
        rootY += top->y_root();
    }
    return {rootX, rootY};
}

inline bool rootPointInsideWidget(const Fl_Widget* widget, int rootX, int rootY) {
    if (!widget || !widget->visible_r()) return false;

    const auto [widgetX, widgetY] = rootPosition(widget);
    return rootX >= widgetX &&
           rootX < widgetX + widget->w() &&
           rootY >= widgetY &&
           rootY < widgetY + widget->h();
}

inline PopupOwner* popupOwnerAtRootPoint(int rootX, int rootY) {
    auto& owners = popupOwners();
    for (auto it = owners.rbegin(); it != owners.rend(); ++it) {
        PopupOwner* owner = *it;
        if (!owner) continue;

        Fl_Widget* anchor = owner->popupAnchorWidget();
        if (!anchor || !anchor->active_r() || !anchor->visible_r()) continue;
        if (rootPointInsideWidget(anchor, rootX, rootY)) {
            return owner;
        }
    }
    return nullptr;
}

inline std::string displayLabelForMenuText(const char* text) {
    if (!text) return "";

    std::string label;
    label.reserve(std::char_traits<char>::length(text));

    bool escaped = false;
    for (const char* p = text; *p; ++p) {
        const char c = *p;
        if (escaped) {
            label.push_back(c);
            escaped = false;
            continue;
        }

        if (c == '\\') {
            escaped = true;
            continue;
        }

        if (c == '&') {
            if (p[1] == '&') {
                label.push_back('&');
                ++p;
            }
            continue;
        }

        label.push_back(c);
    }

    if (escaped) {
        label.push_back('\\');
    }

    return label;
}

inline int measureLabelWidth(Fl_Font font, Fl_Fontsize size, const std::string& label) {
    fl_font(font, size);
    int width = 0;
    int height = 0;
    fl_measure(label.c_str(), width, height, 0);
    return width;
}

inline int maxColumnWidthForWorkArea(int workW) {
    return std::clamp(workW / 3,
                      kPopupDefaultMinCappedColumnW,
                      kPopupDefaultMaxColumnW);
}

inline int clampPopupColumnWidth(int desiredWidth, int workW, int minWidth) {
    return std::clamp(desiredWidth,
                      minWidth,
                      std::max(minWidth, maxColumnWidthForWorkArea(workW)));
}

inline void positionBrowserForOpen(Fl_Hold_Browser* browser, int selectedLine) {
    if (!browser || browser->size() <= 0) return;

    browser->topline(1);

    if (selectedLine <= 0) return;

    selectedLine = std::clamp(selectedLine, 1, browser->size());
    if (!browser->displayed(browser->size())) {
        browser->middleline(selectedLine);
    }
}

} // namespace verdad::choice_popup

#endif // VERDAD_CHOICE_POPUP_UTILS_H
