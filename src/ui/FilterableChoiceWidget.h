#ifndef VERDAD_FILTERABLE_CHOICE_WIDGET_H
#define VERDAD_FILTERABLE_CHOICE_WIDGET_H

#include <FL/Fl_Input_Choice.H>

#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace verdad {

class FilterableChoiceWidget : public Fl_Input_Choice {
public:
    FilterableChoiceWidget(int X, int Y, int W, int H, const char* label = nullptr);
    ~FilterableChoiceWidget() override;

    void setItems(const std::vector<std::string>& items);
    void setItemsView(const std::vector<std::string>* items);
    void setSelectedValue(const std::string& value);
    void setDisplayedValue(const std::string& value);
    std::string selectedValue() const;

    void setNoMatchesLabel(const std::string& label);
    void setShowAllWhenFilterEmpty(bool showAll);
    void setMaxVisibleItems(size_t maxItems);
    void setEnsureItemsCallback(std::function<void()> callback);

private:
    std::vector<std::string> ownedItems_;
    const std::vector<std::string>* items_ = &ownedItems_;
    std::string selectedItem_;
    std::string noMatchesLabel_ = "No matches";
    bool showAllWhenFilterEmpty_ = true;
    size_t maxVisibleItems_ = std::numeric_limits<size_t>::max();
    std::function<void()> ensureItemsCallback_;

    void refreshMenu(const std::string& filter);
    std::string exactItemMatch(const std::string& value) const;
    const std::vector<std::string>& items() const;
    void clearMenu();

    static void onInputChanged(Fl_Widget* w, void* data);
    static void onMenuSelected(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_FILTERABLE_CHOICE_WIDGET_H
