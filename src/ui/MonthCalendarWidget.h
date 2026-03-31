#ifndef VERDAD_MONTH_CALENDAR_WIDGET_H
#define VERDAD_MONTH_CALENDAR_WIDGET_H

#include <FL/Fl_Widget.H>

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "reading/DateUtils.h"

namespace verdad {

struct CalendarDayMeta {
    std::string summary;
    bool hasContent = false;
    bool completed = false;
    bool overdue = false;
};

class MonthCalendarWidget : public Fl_Widget {
public:
    using DateSelectCallback = std::function<void(const reading::Date&)>;

    MonthCalendarWidget(int X, int Y, int W, int H, const char* label = nullptr);
    ~MonthCalendarWidget() override = default;

    void setDisplayedMonth(const reading::Date& date);
    reading::Date displayedMonth() const { return displayedMonth_; }

    void setSelectedDate(const reading::Date& date);
    reading::Date selectedDate() const { return selectedDate_; }
    void setSelectedDates(const std::vector<std::string>& dateIsos);
    std::vector<std::string> selectedDateIsos() const;
    bool hasSelectedDateIso(const std::string& dateIso) const;

    void setDayMeta(const std::unordered_map<std::string, CalendarDayMeta>& metaByIso);
    void setDateSelectCallback(DateSelectCallback callback);

    int handle(int event) override;
    void draw() override;

private:
    struct Cell {
        reading::Date date;
        bool inDisplayedMonth = false;
        int X = 0;
        int Y = 0;
        int W = 0;
        int H = 0;
    };

    reading::Date displayedMonth_;
    reading::Date selectedDate_;
    std::vector<std::string> selectedDateIsos_;
    std::unordered_set<std::string> selectedDateSet_;
    std::unordered_map<std::string, CalendarDayMeta> metaByIso_;
    DateSelectCallback selectCallback_;
    bool draggingSelection_ = false;
    reading::Date dragAnchorDate_;

    Cell cellAt(int mouseX, int mouseY) const;
    void buildCells(Cell cells[42]) const;
    void setSelectionRange(const reading::Date& from, const reading::Date& to);
};

} // namespace verdad

#endif // VERDAD_MONTH_CALENDAR_WIDGET_H
