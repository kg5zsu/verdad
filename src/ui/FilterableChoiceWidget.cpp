#include "ui/ChoicePopupUtils.h"
#include "ui/FilterableChoiceWidget.h"

#include <FL/Fl.H>
#include <FL/Fl_Browser_.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Menu_Button.H>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string_view>
#include <utility>

namespace verdad {

class PopupListBrowser : public Fl_Hold_Browser {
public:
    PopupListBrowser(FilterableChoiceWidget* owner, int X, int Y, int W, int H)
        : Fl_Hold_Browser(X, Y, W, H)
        , owner_(owner) {}

    int handle(int event) override {
        if (event == FL_PUSH) {
            const int button = Fl::event_button();
            if (button == FL_LEFT_MOUSE || button == FL_MIDDLE_MOUSE) {
                pressedLine_ = lineAtEvent();
            } else {
                pressedLine_ = 0;
            }
        }

        const int button = (event == FL_RELEASE) ? Fl::event_button() : 0;
        const int line = (event == FL_RELEASE) ? lineAtEvent() : 0;
        const int handled = Fl_Hold_Browser::handle(event);

        if (event == FL_RELEASE && owner_ &&
            (button == FL_LEFT_MOUSE || button == FL_MIDDLE_MOUSE)) {
            if (!owner_->popupVisible()) {
                pressedLine_ = 0;
                return 1;
            }

            int targetLine = line;
            if (pressedLine_ > 0 && (targetLine <= 0 || targetLine == pressedLine_)) {
                targetLine = pressedLine_;
            }
            if (targetLine <= 0) {
                targetLine = value();
            }

            owner_->selectPopupLine(targetLine, true);
            pressedLine_ = 0;
            return 1;
        }

        return handled;
    }

private:
    int lineAtEvent() {
        void* item = find_item(Fl::event_y());
        return item ? lineno(item) : 0;
    }

    FilterableChoiceWidget* owner_ = nullptr;
    int pressedLine_ = 0;
};

class PopupListWindow : public Fl_Double_Window {
public:
    PopupListWindow(FilterableChoiceWidget* owner, int X, int Y, int W, int H)
        : Fl_Double_Window(X, Y, W, H)
        , owner_(owner) {
        box(FL_NO_BOX);
        clear_border();
    }

    ~PopupListWindow() override {
        if (owner_) owner_->onPopupDestroyed();
    }

private:
    FilterableChoiceWidget* owner_ = nullptr;
};

namespace {

constexpr int kPopupMaxRows = 10;
constexpr int kPopupMinWidth = 120;
constexpr int kPopupWidthPadding = 24;

std::string_view trimView(std::string_view text) {
    size_t start = 0;
    while (start < text.size() &&
           std::isspace(static_cast<unsigned char>(text[start]))) {
        ++start;
    }

    size_t end = text.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(text[end - 1]))) {
        --end;
    }

    return text.substr(start, end - start);
}

std::string normalizeMatchText(std::string_view text) {
    std::string_view trimmed = trimView(text);
    if (trimmed.empty()) return "";

    std::string lowered(trimmed.size() * 4 + 4, '\0');
    int loweredLen = fl_utf_tolower(
        reinterpret_cast<const unsigned char*>(trimmed.data()),
        static_cast<int>(trimmed.size()),
        lowered.data());
    if (loweredLen <= 0) {
        std::string fallback(trimmed);
        std::transform(fallback.begin(), fallback.end(), fallback.begin(),
                       [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        return fallback;
    }

    lowered.resize(static_cast<size_t>(loweredLen));
    return lowered;
}

std::string browserLineLabel(const std::string& label) {
    return "@." + label;
}

int desiredPopupWidth(const FilterableChoiceWidget* widget,
                      const std::vector<std::string>& labels) {
    if (!widget) return kPopupMinWidth;

    int width = std::max(widget->w(), kPopupMinWidth);
    for (const auto& label : labels) {
        width = std::max(width,
                         choice_popup::measureLabelWidth(widget->textfont(),
                                                         widget->textsize(),
                                                         label) +
                             kPopupWidthPadding);
    }
    return width;
}

std::vector<FilterableChoiceWidget*>& liveWidgets() {
    static std::vector<FilterableChoiceWidget*> widgets;
    return widgets;
}

FilterableChoiceWidget*& openPopupOwner() {
    static FilterableChoiceWidget* owner = nullptr;
    return owner;
}

bool& globalHandlerRegistered() {
    static bool registered = false;
    return registered;
}

bool& dispatchRegistered() {
    static bool registered = false;
    return registered;
}

Fl_Event_Dispatch& previousDispatch() {
    static Fl_Event_Dispatch dispatch = nullptr;
    return dispatch;
}

} // namespace

FilterableChoiceWidget::FilterableChoiceWidget(int X, int Y, int W, int H,
                                               const char* label)
    : Fl_Input_Choice(X, Y, W, H, label) {
    input()->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
    input()->callback(onInputChanged, this);
    clearMenu();

    liveWidgets().push_back(this);
    registerGlobalHandler();
}

FilterableChoiceWidget::~FilterableChoiceWidget() {
    hidePopup();
    destroyPopup();

    auto& widgets = liveWidgets();
    widgets.erase(std::remove(widgets.begin(), widgets.end(), this), widgets.end());
    unregisterGlobalHandler();
}

int FilterableChoiceWidget::handle(int event) {
    if (event == FL_PUSH && eventInMenuButton()) {
        showPopup();
        if (input()) input()->take_focus();
        return 1;
    }

    if ((event == FL_KEYBOARD || event == FL_SHORTCUT) &&
        Fl::focus() == menubutton()) {
        const int key = Fl::event_key();
        if (key == ' ' || key == FL_Down || key == FL_Up) {
            showPopup();
            if (input()) input()->take_focus();
            if (popupVisible() && key == FL_Up) {
                movePopupSelection(-1);
            } else if (popupVisible() && key == FL_Down) {
                movePopupSelection(1);
            }
            return 1;
        }
    }

    return Fl_Input_Choice::handle(event);
}

void FilterableChoiceWidget::resize(int X, int Y, int W, int H) {
    Fl_Input_Choice::resize(X, Y, W, H);
    if (popupVisible()) updatePopupGeometry();
}

void FilterableChoiceWidget::setItems(const std::vector<std::string>& items) {
    ownedItems_ = items;
    items_ = &ownedItems_;
    rebuildItemIndex();
    if (!selectedItem_.empty() && exactItemMatch(selectedItem_).empty()) {
        selectedItem_.clear();
    }
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setItemsView(const std::vector<std::string>* items) {
    items_ = items ? items : &ownedItems_;
    rebuildItemIndex();
    if (!selectedItem_.empty() && exactItemMatch(selectedItem_).empty()) {
        selectedItem_.clear();
    }
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setSelectedValue(const std::string& value) {
    selectedItem_ = exactItemMatch(value);
    committedValue_ = value;
    Fl_Input_Choice::value(value.c_str());
    hidePopup();
    refreshMenu(value);
}

void FilterableChoiceWidget::setDisplayedValue(const std::string& value) {
    selectedItem_ = exactItemMatch(value);
    committedValue_ = value;
    Fl_Input_Choice::value(value.c_str());
    hidePopup();
    clearMenu();
}

std::string FilterableChoiceWidget::selectedValue() const {
    std::string typed = Fl_Input_Choice::value() ? Fl_Input_Choice::value() : "";
    std::string exact = exactItemMatch(typed);
    if (!exact.empty()) return exact;
    return selectedItem_;
}

std::string FilterableChoiceWidget::matchedValue(const std::string& value) const {
    return exactItemMatch(value);
}

void FilterableChoiceWidget::setNoMatchesLabel(const std::string& label) {
    noMatchesLabel_ = label;
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setShowAllWhenFilterEmpty(bool showAll) {
    showAllWhenFilterEmpty_ = showAll;
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setMaxVisibleItems(size_t maxItems) {
    if (maxItems == 0) {
        maxVisibleItems_ = std::numeric_limits<size_t>::max();
    } else {
        maxVisibleItems_ = maxItems;
    }
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setEnsureItemsCallback(std::function<void()> callback) {
    ensureItemsCallback_ = std::move(callback);
}

const std::vector<std::string>& FilterableChoiceWidget::items() const {
    return items_ ? *items_ : ownedItems_;
}

void FilterableChoiceWidget::clearMenu() {
    Fl_Menu_Button* menuButton = menubutton();
    if (!menuButton) return;

    menuButton->clear();
    menuButton->value(-1);
}

void FilterableChoiceWidget::destroyPopup() {
    if (!popupWindow_) return;

    Fl_Widget* popup = popupWindow_;
    popupBrowser_ = nullptr;
    popupWindow_ = nullptr;
    if (popup->parent()) {
        popup->parent()->remove(*popup);
    }
    delete popup;
}

void FilterableChoiceWidget::refreshMenu(const std::string& filter) {
    clearMenu();
    if (popupVisible()) refreshPopupContents(filter);
}

void FilterableChoiceWidget::ensureItemsLoaded() {
    if (items().empty() && ensureItemsCallback_) {
        ensureItemsCallback_();
    }
    ensureItemIndex();
}

void FilterableChoiceWidget::ensureItemIndex() {
    if (indexedItems_ == items_ && normalizedItems_.size() == items().size()) {
        return;
    }
    rebuildItemIndex();
}

void FilterableChoiceWidget::rebuildItemIndex() {
    indexedItems_ = items_;
    normalizedItems_.clear();
    exactItemIndex_.clear();

    const auto& sourceItems = items();
    normalizedItems_.reserve(sourceItems.size());
    exactItemIndex_.reserve(sourceItems.size());
    for (size_t i = 0; i < sourceItems.size(); ++i) {
        std::string normalized = normalizeMatchText(sourceItems[i]);
        normalizedItems_.push_back(normalized);
        if (!normalized.empty() &&
            exactItemIndex_.find(normalized) == exactItemIndex_.end()) {
            exactItemIndex_.emplace(std::move(normalized), i);
        }
    }
}

void FilterableChoiceWidget::ensurePopupCreated() {
    if (popupWindow_) return;

    Fl_Window* hostWindow = window();
    if (!hostWindow) return;

    Fl_Group* savedCurrent = Fl_Group::current();
    Fl_Group::current(hostWindow);
    popupWindow_ = new PopupListWindow(this, x(), y() + h() - 1, w(), 1);
    popupWindow_->begin();
    popupBrowser_ = new PopupListBrowser(this, 0, 0, w(), 1);
    popupBrowser_->box(FL_DOWN_BOX);
    popupBrowser_->has_scrollbar(Fl_Browser_::VERTICAL);
    popupBrowser_->when(FL_WHEN_CHANGED);
    popupBrowser_->callback(onPopupSelected, this);
    popupWindow_->resizable(popupBrowser_);
    popupWindow_->end();
    popupWindow_->hide();
    Fl_Group::current(savedCurrent);
}

void FilterableChoiceWidget::refreshPopupContents(const std::string& filter) {
    if (!popupBrowser_) return;

    popupBrowser_->textfont(input()->textfont());
    popupBrowser_->textsize(input()->textsize());
    popupBrowser_->clear();
    popupItems_.clear();

    ensureItemIndex();
    std::string normalizedFilter = normalizeMatchText(filter);
    if (normalizedFilter.empty() && !showAllWhenFilterEmpty_) {
        if (!emptyFilterPrompt_.empty()) {
            popupBrowser_->add(browserLineLabel(emptyFilterPrompt_).c_str());
        }
        popupBrowser_->value(0);
        updatePopupGeometry();
        choice_popup::positionBrowserForOpen(popupBrowser_, 0);
        return;
    }

    const auto& sourceItems = items();
    for (size_t i = 0; i < sourceItems.size(); ++i) {
        if (normalizedFilter.empty() ||
            normalizedItems_[i].find(normalizedFilter) != std::string::npos) {
            if (popupItems_.size() >= maxVisibleItems_) break;
            popupItems_.push_back(sourceItems[i]);
            popupBrowser_->add(browserLineLabel(sourceItems[i]).c_str());
        }
    }

    if (popupItems_.empty()) {
        if (!noMatchesLabel_.empty()) {
            popupBrowser_->add(browserLineLabel(noMatchesLabel_).c_str());
        }
        popupBrowser_->value(0);
        updatePopupGeometry();
        choice_popup::positionBrowserForOpen(popupBrowser_, 0);
        return;
    }

    std::string exact = exactItemMatch(
        input() && input()->value() ? input()->value() : "");

    int selectedLine = 1;
    const std::string& preferred = !exact.empty() ? exact : selectedItem_;
    if (!preferred.empty()) {
        auto it = std::find(popupItems_.begin(), popupItems_.end(), preferred);
        if (it != popupItems_.end()) {
            selectedLine = static_cast<int>(std::distance(popupItems_.begin(), it)) + 1;
        }
    }
    popupBrowser_->value(selectedLine);
    updatePopupGeometry();
    choice_popup::positionBrowserForOpen(popupBrowser_, selectedLine);
}

std::string FilterableChoiceWidget::exactItemMatch(const std::string& value) const {
    auto* self = const_cast<FilterableChoiceWidget*>(this);
    self->ensureItemIndex();

    std::string wanted = normalizeMatchText(value);
    if (wanted.empty()) return "";

    auto it = exactItemIndex_.find(wanted);
    if (it == exactItemIndex_.end()) return "";

    const auto& sourceItems = items();
    if (it->second >= sourceItems.size()) {
        return "";
    }
    return sourceItems[it->second];
}

void FilterableChoiceWidget::updatePopupGeometry() {
    if (!popupWindow_ || !popupBrowser_ || !window()) return;

    const int lineCount = std::max(1, popupBrowser_->size());
    const int visibleRows = std::min(lineCount, kPopupMaxRows);
    const int lineHeight = std::max(20, static_cast<int>(popupBrowser_->textsize()) + 8);
    std::vector<std::string> visibleLabels = popupItems_;
    if (visibleLabels.empty()) {
        if (!emptyFilterPrompt_.empty()) {
            visibleLabels.push_back(emptyFilterPrompt_);
        } else if (!noMatchesLabel_.empty()) {
            visibleLabels.push_back(noMatchesLabel_);
        }
    }
    int popupW = desiredPopupWidth(this, visibleLabels);
    int popupH = std::max(lineHeight + 4, (visibleRows * lineHeight) + 4);

    const int hostW = std::max(1, window()->w());
    const int hostH = std::max(1, window()->h());
    popupW = choice_popup::clampPopupColumnWidth(popupW, hostW, kPopupMinWidth);
    popupW = std::min(popupW, hostW);

    const int popupBelowY = y() + h() - 1;
    const int spaceBelow = std::max(0, hostH - popupBelowY);
    const int spaceAbove = std::max(0, y());
    const bool placeBelow = (spaceBelow >= popupH) || (spaceBelow >= spaceAbove);
    if (placeBelow && spaceBelow > 0) {
        popupH = std::min(popupH, spaceBelow);
    } else if (!placeBelow && spaceAbove > 0) {
        popupH = std::min(popupH, spaceAbove);
    }
    popupH = std::max(lineHeight + 4, popupH);

    const int maxPopupX = std::max(0, hostW - popupW);
    const int popupX = std::clamp(x(), 0, maxPopupX);
    const int popupY = placeBelow ? popupBelowY : y() - popupH + 1;

    popupWindow_->resize(popupX, popupY, popupW, popupH);
    popupBrowser_->resize(0, 0, popupW, popupH);
}

bool FilterableChoiceWidget::showPopup() {
    if (!window() || !visible_r() || !active_r()) return false;

    ensureItemsLoaded();
    ensurePopupCreated();
    if (!popupWindow_) return false;
    if (openPopupOwner() && openPopupOwner() != this) {
        openPopupOwner()->hidePopup();
    }

    refreshPopupContents(input() && input()->value() ? input()->value() : "");
    if (!popupBrowser_ || popupBrowser_->size() <= 0) {
        hidePopup();
        return false;
    }

    updatePopupGeometry();
    choice_popup::positionBrowserForOpen(popupBrowser_, popupSelectionLine());
    openPopupOwner() = this;
    if (popupWindow_->parent() == window()) {
        window()->remove(*popupWindow_);
        window()->add(*popupWindow_);
    }
    popupWindow_->show();
    if (input()) input()->take_focus();
    popupWindow_->redraw();
    if (window()) window()->redraw();
    return true;
}

void FilterableChoiceWidget::hidePopup() {
    if (openPopupOwner() == this) {
        openPopupOwner() = nullptr;
    }
    if (popupWindow_ && popupWindow_->shown()) {
        popupWindow_->hide();
        if (window()) window()->redraw();
    }
}

void FilterableChoiceWidget::cancelEditing() {
    if (!popupVisible() && !hasPendingEdits()) return;

    hidePopup();
    Fl_Input_Choice::value(committedValue_.c_str());
    if (input()) {
        const int cursor = static_cast<int>(committedValue_.size());
        input()->insert_position(cursor);
        input()->mark(cursor);
        input()->redraw();
    }
}

void FilterableChoiceWidget::onPopupDestroyed() {
    popupBrowser_ = nullptr;
    popupWindow_ = nullptr;
}

bool FilterableChoiceWidget::popupVisible() const {
    return popupWindow_ && popupWindow_->shown() && openPopupOwner() == this;
}

int FilterableChoiceWidget::popupSelectionLine() const {
    if (!popupBrowser_ || popupItems_.empty()) return 0;

    int line = popupBrowser_->value();
    if (line <= 0 || line > static_cast<int>(popupItems_.size())) return 0;
    return line;
}

void FilterableChoiceWidget::movePopupSelection(int delta) {
    if (!popupBrowser_ || popupItems_.empty() || delta == 0) return;

    int line = popupSelectionLine();
    if (line <= 0) {
        line = (delta > 0) ? 1 : static_cast<int>(popupItems_.size());
    } else {
        line = std::clamp(line + delta,
                          1,
                          static_cast<int>(popupItems_.size()));
    }

    popupBrowser_->value(line);
    popupBrowser_->make_visible(line);
    popupBrowser_->redraw();
}

void FilterableChoiceWidget::selectPopupLine(int line, bool invokeCallback) {
    if (line <= 0 || line > static_cast<int>(popupItems_.size())) return;

    const std::string selected = popupItems_[static_cast<size_t>(line - 1)];
    selectedItem_ = selected;
    Fl_Input_Choice::value(selected.c_str());
    hidePopup();

    if (input()) {
        input()->insert_position(static_cast<int>(selected.size()));
        input()->mark(static_cast<int>(selected.size()));
        input()->take_focus();
    }

    if (invokeCallback) {
        do_callback(FL_REASON_CHANGED);
    }
}

bool FilterableChoiceWidget::handlePopupKey(int key) {
    switch (key) {
    case FL_Escape:
        cancelEditing();
        return true;
    case FL_Down:
        movePopupSelection(1);
        return true;
    case FL_Up:
        movePopupSelection(-1);
        return true;
    case FL_Page_Down:
        movePopupSelection(kPopupMaxRows);
        return true;
    case FL_Page_Up:
        movePopupSelection(-kPopupMaxRows);
        return true;
    case FL_Home:
        if (!popupItems_.empty()) movePopupSelection(-static_cast<int>(popupItems_.size()));
        return true;
    case FL_End:
        if (!popupItems_.empty()) movePopupSelection(static_cast<int>(popupItems_.size()));
        return true;
    case FL_Enter:
    case FL_KP_Enter: {
        int line = popupSelectionLine();
        if (line <= 0 && !popupItems_.empty()) line = 1;
        if (line > 0) {
            selectPopupLine(line, true);
        } else {
            hidePopup();
            do_callback(FL_REASON_ENTER_KEY);
        }
        return true;
    }
    default:
        return false;
    }
}

bool FilterableChoiceWidget::hasPendingEdits() const {
    const char* currentValue = Fl_Input_Choice::value();
    const std::string current = currentValue ? currentValue : "";
    return current != committedValue_;
}

bool FilterableChoiceWidget::ownsFocus() {
    Fl_Widget* focus = Fl::focus();
    return focus == this || focus == input() || focus == menubutton();
}

bool FilterableChoiceWidget::containsRootPoint(int rootX, int rootY) const {
    return choice_popup::rootPointInsideWidget(this, rootX, rootY);
}

bool FilterableChoiceWidget::popupContainsRootPoint(int rootX, int rootY) const {
    if (!popupWindow_ || !popupWindow_->shown()) return false;

    return rootX >= popupWindow_->x_root() &&
           rootX < popupWindow_->x_root() + popupWindow_->w() &&
           rootY >= popupWindow_->y_root() &&
           rootY < popupWindow_->y_root() + popupWindow_->h();
}

bool FilterableChoiceWidget::menuButtonContainsRootPoint(int rootX, int rootY) {
    return choice_popup::rootPointInsideWidget(menubutton(), rootX, rootY);
}

bool FilterableChoiceWidget::eventInMenuButton() {
    const int rootX = Fl::event_x_root();
    const int rootY = Fl::event_y_root();
    return popupVisible() ? menuButtonContainsRootPoint(rootX, rootY)
                          : choice_popup::rootPointInsideWidget(menubutton(), rootX, rootY);
}

FilterableChoiceWidget* FilterableChoiceWidget::widgetAtRootPoint(int rootX, int rootY) {
    auto& widgets = liveWidgets();
    for (auto it = widgets.rbegin(); it != widgets.rend(); ++it) {
        FilterableChoiceWidget* widget = *it;
        if (!widget || !widget->active_r() || !widget->visible_r()) continue;
        if (widget->containsRootPoint(rootX, rootY)) return widget;
    }
    return nullptr;
}

FilterableChoiceWidget* FilterableChoiceWidget::widgetWithFocus() {
    Fl_Widget* focus = Fl::focus();
    if (!focus) return nullptr;

    auto& widgets = liveWidgets();
    for (auto it = widgets.rbegin(); it != widgets.rend(); ++it) {
        FilterableChoiceWidget* widget = *it;
        if (!widget || !widget->active_r() || !widget->visible_r()) continue;
        if (focus == widget || focus == widget->input() || focus == widget->menubutton()) {
            return widget;
        }
    }
    return nullptr;
}

int FilterableChoiceWidget::dispatchEvent(int event, Fl_Window* window) {
    FilterableChoiceWidget* openWidget = openPopupOwner();
    if (event == FL_PUSH && openWidget && openWidget->popupVisible()) {
        const int rootX = Fl::event_x_root();
        const int rootY = Fl::event_y_root();
        if (!openWidget->containsRootPoint(rootX, rootY) &&
            !openWidget->popupContainsRootPoint(rootX, rootY)) {
            openWidget->cancelEditing();
        }
    }

    if (event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
        const int rootX = Fl::event_x_root();
        const int rootY = Fl::event_y_root();
        FilterableChoiceWidget* target = widgetAtRootPoint(rootX, rootY);
        if (target && target->input() &&
            choice_popup::rootPointInsideWidget(target->input(), rootX, rootY) &&
            Fl::focus() != target->input()) {
            const char* currentValue = target->input()->value();
            const int textLength = currentValue ? static_cast<int>(std::strlen(currentValue)) : 0;
            target->input()->take_focus();
            target->input()->insert_position(textLength, 0);
            target->input()->redraw();
            return 1;
        }
    }

    if ((event == FL_KEYBOARD || event == FL_SHORTCUT) &&
        openWidget && openWidget->popupVisible() && openWidget->ownsFocus()) {
        const int key = Fl::event_key();
        if (key == FL_Tab) {
            openWidget->cancelEditing();
        } else if (openWidget->handlePopupKey(key)) {
            return 1;
        }
    }

    if ((event == FL_KEYBOARD || event == FL_SHORTCUT) && !openWidget) {
        FilterableChoiceWidget* focusedWidget = widgetWithFocus();
        if (focusedWidget && Fl::event_key() == FL_Escape &&
            focusedWidget->hasPendingEdits()) {
            focusedWidget->cancelEditing();
            return 1;
        }
        if (focusedWidget && (Fl::event_key() == FL_Down || Fl::event_key() == FL_Up)) {
            if (focusedWidget->showPopup()) {
                focusedWidget->movePopupSelection(Fl::event_key() == FL_Down ? 1 : -1);
                return 1;
            }
        }
    }

    Fl_Event_Dispatch dispatch = previousDispatch();
    return dispatch ? dispatch(event, window) : Fl::handle_(event, window);
}

void FilterableChoiceWidget::registerGlobalHandler() {
    if (globalHandlerRegistered()) return;
    Fl::add_handler(handleGlobalEvent);
    globalHandlerRegistered() = true;
    if (!dispatchRegistered()) {
        previousDispatch() = Fl::event_dispatch();
        Fl::event_dispatch(dispatchEvent);
        dispatchRegistered() = true;
    }
}

void FilterableChoiceWidget::unregisterGlobalHandler() {
    if (!globalHandlerRegistered() || !liveWidgets().empty()) return;
    Fl::remove_handler(handleGlobalEvent);
    globalHandlerRegistered() = false;
    if (dispatchRegistered() && Fl::event_dispatch() == dispatchEvent) {
        Fl::event_dispatch(previousDispatch());
    }
    previousDispatch() = nullptr;
    dispatchRegistered() = false;
}

int FilterableChoiceWidget::handleGlobalEvent(int event) {
    if (event == FL_PUSH) {
        const int rootX = Fl::event_x_root();
        const int rootY = Fl::event_y_root();
        FilterableChoiceWidget* openWidget = openPopupOwner();
        if (openWidget && openWidget->popupContainsRootPoint(rootX, rootY)) {
            return 0;
        }

        FilterableChoiceWidget* target = widgetAtRootPoint(rootX, rootY);
        if (!target || Fl::event_button() != FL_LEFT_MOUSE) return 0;

        const bool hadFocus = target->ownsFocus();
        const bool clickedMenuButton = target->menuButtonContainsRootPoint(rootX, rootY);
        const bool clickedInput = choice_popup::rootPointInsideWidget(target->input(),
                                                                      rootX,
                                                                      rootY);
        target->showPopup();
        if (clickedMenuButton) {
            if (target->input()) target->input()->take_focus();
            return 1;
        }
        if (clickedInput && !hadFocus && target->input()) {
            const char* currentValue = target->input()->value();
            const int textLength = currentValue ? static_cast<int>(std::strlen(currentValue)) : 0;
            target->input()->take_focus();
            target->input()->insert_position(textLength, 0);
            target->input()->redraw();
            return 1;
        }
        return 0;
    }

    if (event == FL_KEYBOARD || event == FL_SHORTCUT) {
        FilterableChoiceWidget* openWidget = openPopupOwner();
        if (!openWidget || !openWidget->popupVisible()) return 0;

        const int key = Fl::event_key();
        if (key == FL_Escape && openWidget->ownsFocus()) {
            openWidget->cancelEditing();
            return 1;
        }
        if (key == FL_Tab) {
            openWidget->cancelEditing();
        }
    }

    return 0;
}

void FilterableChoiceWidget::onInputChanged(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<FilterableChoiceWidget*>(data);
    if (!self) return;

    self->ensureItemsLoaded();

    std::string filter = self->input() && self->input()->value()
        ? self->input()->value()
        : "";
    if (self->popupVisible()) {
        self->refreshPopupContents(filter);
    } else if (self->ownsFocus()) {
        self->showPopup();
    }

    const int key = Fl::event_key();
    if (Fl::event() == FL_KEYBOARD &&
        (key == FL_Enter || key == FL_KP_Enter)) {
        if (self->popupVisible() && !self->popupItems_.empty()) {
            int line = self->popupSelectionLine();
            if (line <= 0) line = 1;
            self->selectPopupLine(line, true);
        } else {
            self->hidePopup();
            self->do_callback(FL_REASON_ENTER_KEY);
        }
    }
}

void FilterableChoiceWidget::onPopupSelected(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<FilterableChoiceWidget*>(data);
    if (!self || !self->popupBrowser_) return;

    self->selectPopupLine(self->popupBrowser_->value(), true);
}

} // namespace verdad
