#include "ui/FilterableChoiceWidget.h"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Button.H>

#include <algorithm>
#include <cctype>

namespace verdad {
namespace {

std::string trimCopy(const std::string& text) {
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

std::string lowerAsciiCopy(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return text;
}

bool containsNoCase(const std::string& haystack, const std::string& needle) {
    if (needle.empty()) return true;
    return lowerAsciiCopy(haystack).find(lowerAsciiCopy(needle)) != std::string::npos;
}

std::string escapeChoiceLabel(const std::string& label) {
    std::string escaped;
    escaped.reserve(label.size());
    for (char c : label) {
        if (c == '/') escaped.push_back('\\');
        escaped.push_back(c);
    }
    return escaped;
}

std::string unescapeChoiceLabel(const char* label) {
    if (!label) return "";

    std::string unescaped;
    for (size_t i = 0; label[i] != '\0'; ++i) {
        if (label[i] == '\\' && label[i + 1] == '/') {
            unescaped.push_back('/');
            ++i;
            continue;
        }
        unescaped.push_back(label[i]);
    }
    return unescaped;
}

int findMenuIndexByLabel(Fl_Menu_Button* menuButton, const std::string& label) {
    if (!menuButton || label.empty()) return -1;

    for (int i = 0; i < menuButton->size(); ++i) {
        const Fl_Menu_Item* item = menuButton->menu()
            ? &menuButton->menu()[i]
            : nullptr;
        if (!item || !item->label()) continue;
        if (unescapeChoiceLabel(item->label()) == label) return i;
    }

    return -1;
}

} // namespace

FilterableChoiceWidget::FilterableChoiceWidget(int X, int Y, int W, int H,
                                               const char* label)
    : Fl_Input_Choice(X, Y, W, H, label) {
    input()->when(FL_WHEN_CHANGED | FL_WHEN_ENTER_KEY_ALWAYS);
    input()->callback(onInputChanged, this);
    menubutton()->callback(onMenuSelected, this);
}

FilterableChoiceWidget::~FilterableChoiceWidget() = default;

void FilterableChoiceWidget::setItems(const std::vector<std::string>& items) {
    items_ = items;
    if (!selectedItem_.empty() && exactItemMatch(selectedItem_).empty()) {
        selectedItem_.clear();
    }
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setSelectedValue(const std::string& value) {
    selectedItem_ = exactItemMatch(value);
    Fl_Input_Choice::value(value.c_str());
    refreshMenu(value);
}

std::string FilterableChoiceWidget::selectedValue() const {
    std::string typed = Fl_Input_Choice::value() ? Fl_Input_Choice::value() : "";
    std::string exact = exactItemMatch(typed);
    if (!exact.empty()) return exact;
    return selectedItem_;
}

void FilterableChoiceWidget::setNoMatchesLabel(const std::string& label) {
    noMatchesLabel_ = label;
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::setShowAllWhenFilterEmpty(bool showAll) {
    showAllWhenFilterEmpty_ = showAll;
    refreshMenu(input() && input()->value() ? input()->value() : "");
}

void FilterableChoiceWidget::refreshMenu(const std::string& filter) {
    Fl_Menu_Button* menuButton = menubutton();
    if (!menuButton) return;

    std::string normalizedFilter = trimCopy(filter);
    if (normalizedFilter.empty() && !showAllWhenFilterEmpty_) {
        menuButton->clear();
        menuButton->deactivate();
        return;
    }

    std::vector<std::string> filtered;
    filtered.reserve(items_.size());
    for (const auto& item : items_) {
        if (normalizedFilter.empty() ||
            containsNoCase(item, normalizedFilter)) {
            filtered.push_back(item);
        }
    }

    menuButton->clear();
    if (filtered.empty()) {
        if (!noMatchesLabel_.empty()) {
            menuButton->add(noMatchesLabel_.c_str());
            menuButton->value(0);
        }
        menuButton->deactivate();
        return;
    }

    menuButton->activate();
    for (const auto& item : filtered) {
        menuButton->add(escapeChoiceLabel(item).c_str());
    }

    std::string exact = exactItemMatch(
        input() && input()->value() ? input()->value() : "");
    if (!exact.empty()) {
        selectedItem_ = exact;
    }

    int selectedIndex = -1;
    if (!selectedItem_.empty()) {
        selectedIndex = findMenuIndexByLabel(menuButton, selectedItem_);
    }
    if (selectedIndex < 0 && menuButton->size() > 0) {
        selectedIndex = 0;
    }
    if (selectedIndex >= 0) {
        menuButton->value(selectedIndex);
    }
}

std::string FilterableChoiceWidget::exactItemMatch(const std::string& value) const {
    std::string wanted = lowerAsciiCopy(trimCopy(value));
    if (wanted.empty()) return "";

    for (const auto& item : items_) {
        if (lowerAsciiCopy(item) == wanted) {
            return item;
        }
    }
    return "";
}

void FilterableChoiceWidget::onInputChanged(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<FilterableChoiceWidget*>(data);
    if (!self) return;

    std::string filter = self->input() && self->input()->value()
        ? self->input()->value()
        : "";
    self->refreshMenu(filter);

    const int key = Fl::event_key();
    if (Fl::event() == FL_KEYBOARD &&
        (key == FL_Enter || key == FL_KP_Enter)) {
        self->do_callback();
    }
}

void FilterableChoiceWidget::onMenuSelected(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<FilterableChoiceWidget*>(data);
    if (!self) return;

    Fl_Menu_Button* menuButton = self->menubutton();
    if (!menuButton) return;

    const Fl_Menu_Item* item = menuButton->mvalue();
    if (!item || !item->label()) return;

    std::string selected = unescapeChoiceLabel(item->label());
    if (!self->noMatchesLabel_.empty() && selected == self->noMatchesLabel_) {
        return;
    }

    self->selectedItem_ = selected;
    self->Fl_Input_Choice::value(selected.c_str());
    self->refreshMenu(selected);
    self->do_callback();
}

} // namespace verdad
