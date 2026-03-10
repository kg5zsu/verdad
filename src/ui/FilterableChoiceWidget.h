#ifndef VERDAD_FILTERABLE_CHOICE_WIDGET_H
#define VERDAD_FILTERABLE_CHOICE_WIDGET_H

#include <FL/Fl_Input_Choice.H>

#include <string>
#include <vector>

namespace verdad {

class FilterableChoiceWidget : public Fl_Input_Choice {
public:
    FilterableChoiceWidget(int X, int Y, int W, int H, const char* label = nullptr);
    ~FilterableChoiceWidget() override;

    void setItems(const std::vector<std::string>& items);
    void setSelectedValue(const std::string& value);
    std::string selectedValue() const;

    void setNoMatchesLabel(const std::string& label);
    void setShowAllWhenFilterEmpty(bool showAll);

private:
    std::vector<std::string> items_;
    std::string selectedItem_;
    std::string noMatchesLabel_ = "No matches";
    bool showAllWhenFilterEmpty_ = true;

    void refreshMenu(const std::string& filter);
    std::string exactItemMatch(const std::string& value) const;

    static void onInputChanged(Fl_Widget* w, void* data);
    static void onMenuSelected(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_FILTERABLE_CHOICE_WIDGET_H
