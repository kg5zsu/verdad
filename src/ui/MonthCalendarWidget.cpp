#include "ui/MonthCalendarWidget.h"

#include "app/VerdadApp.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace verdad {
namespace {

constexpr int kWeekdayHeaderH = 20;
constexpr int kCellPadding = 4;

const char* kWeekdayLabels[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const VerdadApp::ThemePalette& currentThemePalette() {
    static const VerdadApp::ThemePalette fallback;
    if (auto* app = VerdadApp::instance()) return app->themePalette();
    return fallback;
}

Fl_Color summaryColorForCell(const CalendarDayMeta& meta,
                             bool inDisplayedMonth,
                             const VerdadApp::ThemePalette& palette) {
    if (!inDisplayedMonth) return palette.calendarOtherMonthText;
    if (meta.completed) return palette.success;
    if (meta.overdue) return palette.danger;
    return palette.mutedForeground;
}

} // namespace

MonthCalendarWidget::MonthCalendarWidget(int X, int Y, int W, int H, const char* label)
    : Fl_Widget(X, Y, W, H, label)
    , displayedMonth_(reading::today())
    , selectedDate_(reading::today()) {
    box(FL_FLAT_BOX);
    color(FL_BACKGROUND2_COLOR);
    labelcolor(FL_FOREGROUND_COLOR);
    selectedDateIsos_.push_back(reading::formatIsoDate(selectedDate_));
    selectedDateSet_.insert(selectedDateIsos_.front());
}

void MonthCalendarWidget::setDisplayedMonth(const reading::Date& date) {
    if (!date.valid()) return;
    displayedMonth_ = reading::Date{date.year, date.month, 1};
    redraw();
}

void MonthCalendarWidget::setSelectedDate(const reading::Date& date) {
    if (!date.valid()) return;
    selectedDate_ = date;
    displayedMonth_ = reading::Date{date.year, date.month, 1};
    selectedDateIsos_.clear();
    selectedDateIsos_.push_back(reading::formatIsoDate(date));
    selectedDateSet_.clear();
    selectedDateSet_.insert(selectedDateIsos_.front());
    redraw();
}

void MonthCalendarWidget::setSelectedDates(const std::vector<std::string>& dateIsos) {
    selectedDateIsos_.clear();
    selectedDateSet_.clear();
    for (const auto& dateIso : dateIsos) {
        if (!reading::isIsoDateInRange(dateIso)) continue;
        if (!selectedDateSet_.insert(dateIso).second) continue;
        selectedDateIsos_.push_back(dateIso);
    }
    if (selectedDateIsos_.empty()) {
        if (selectedDate_.valid()) {
            selectedDateIsos_.push_back(reading::formatIsoDate(selectedDate_));
            selectedDateSet_.insert(selectedDateIsos_.front());
        }
    } else {
        reading::Date lastDate{};
        if (reading::parseIsoDate(selectedDateIsos_.back(), lastDate)) {
            selectedDate_ = lastDate;
        }
    }
    redraw();
}

std::vector<std::string> MonthCalendarWidget::selectedDateIsos() const {
    return selectedDateIsos_;
}

bool MonthCalendarWidget::hasSelectedDateIso(const std::string& dateIso) const {
    return selectedDateSet_.find(dateIso) != selectedDateSet_.end();
}

void MonthCalendarWidget::setDayMeta(
    const std::unordered_map<std::string, CalendarDayMeta>& metaByIso) {
    metaByIso_ = metaByIso;
    redraw();
}

void MonthCalendarWidget::setDateSelectCallback(DateSelectCallback callback) {
    selectCallback_ = std::move(callback);
}

int MonthCalendarWidget::handle(int event) {
    if (event == FL_PUSH && Fl::event_button() == FL_LEFT_MOUSE) {
        Cell cell = cellAt(Fl::event_x(), Fl::event_y());
        if (cell.date.valid()) {
            draggingSelection_ = true;
            dragAnchorDate_ = cell.date;
            selectedDate_ = cell.date;
            setSelectionRange(cell.date, cell.date);
            return 1;
        }
    } else if (event == FL_DRAG && draggingSelection_) {
        Cell cell = cellAt(Fl::event_x(), Fl::event_y());
        if (cell.date.valid()) {
            if (reading::sameDate(cell.date, selectedDate_)) {
                return 1;
            }
            selectedDate_ = cell.date;
            setSelectionRange(dragAnchorDate_, cell.date);
            return 1;
        }
    } else if (event == FL_RELEASE && draggingSelection_) {
        draggingSelection_ = false;
        if (selectedDate_.valid() && selectCallback_) {
            selectCallback_(selectedDate_);
        }
        return 1;
    }
    return Fl_Widget::handle(event);
}

void MonthCalendarWidget::draw() {
    const auto& palette = currentThemePalette();
    fl_push_clip(x(), y(), w(), h());
    fl_color(color());
    fl_rectf(x(), y(), w(), h());

    const int cellW = std::max(1, w() / 7);
    const int gridTop = y() + kWeekdayHeaderH;
    const int cellH = std::max(32, (h() - kWeekdayHeaderH) / 6);

    fl_font(labelfont(), 11);
    for (int col = 0; col < 7; ++col) {
        int cellX = x() + (col * cellW);
        fl_color(palette.calendarHeaderText);
        fl_draw(kWeekdayLabels[col], cellX, y(), cellW, kWeekdayHeaderH, FL_ALIGN_CENTER);
    }

    Cell cells[42];
    buildCells(cells);

    reading::Date today = reading::today();
    for (const Cell& cell : cells) {
        const std::string iso = reading::formatIsoDate(cell.date);
        auto metaIt = metaByIso_.find(iso);
        CalendarDayMeta meta;
        if (metaIt != metaByIso_.end()) meta = metaIt->second;

        const bool inSelection = hasSelectedDateIso(iso);
        bool isToday = reading::sameDate(cell.date, today);

        Fl_Color fill = cell.inDisplayedMonth
            ? palette.calendarCurrentMonthBackground
            : palette.calendarOtherMonthBackground;
        if (inSelection) {
            fill = palette.tagBackground;
        } else if (meta.completed) {
            fill = palette.successBackground;
        } else if (meta.overdue) {
            fill = palette.dangerBackground;
        }
        fl_color(fill);
        fl_rectf(cell.X, cell.Y, cell.W, cell.H);

        fl_color(palette.calendarGrid);
        fl_rect(cell.X, cell.Y, cell.W, cell.H);

        if (isToday) {
            fl_color(palette.calendarTodayOutline);
            fl_rect(cell.X + 1, cell.Y + 1, cell.W - 2, cell.H - 2);
        }
        if (inSelection) {
            fl_color(palette.calendarRangeOutline);
            fl_rect(cell.X + 2, cell.Y + 2, cell.W - 4, cell.H - 4);
        }
        if (reading::sameDate(cell.date, selectedDate_)) {
            fl_color(palette.calendarSelectedOutline);
            fl_rect(cell.X + 3, cell.Y + 3, cell.W - 6, cell.H - 6);
        }

        fl_color(cell.inDisplayedMonth ? palette.foreground : palette.calendarOtherMonthText);
        fl_font(labelfont(), 12);
        fl_draw(std::to_string(cell.date.day).c_str(),
                cell.X + kCellPadding,
                cell.Y + kCellPadding + 10);

        if (meta.completed) {
            fl_color(palette.success);
            fl_draw("@check", cell.X + cell.W - 18, cell.Y + 2, 16, 16, FL_ALIGN_CENTER);
        } else if (meta.overdue) {
            fl_color(palette.danger);
            fl_draw("!", cell.X + cell.W - 16, cell.Y + 2, 12, 16, FL_ALIGN_CENTER);
        }

        if (!meta.summary.empty()) {
            fl_color(summaryColorForCell(meta, cell.inDisplayedMonth, palette));
            fl_font(FL_HELVETICA, 10);
            const int lineHeight = 12;
            const int maxLines = std::max(1, (cell.H - 22) / lineHeight);
            std::istringstream stream(meta.summary);
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(stream, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) lines.push_back(line);
            }
            int lineIndex = 0;
            while (lineIndex < maxLines && lineIndex < static_cast<int>(lines.size())) {
                std::string clipped = reading::addEllipsis(lines[static_cast<size_t>(lineIndex)],
                                                           20);
                if (lineIndex == maxLines - 1 &&
                    static_cast<int>(lines.size()) > maxLines) {
                    clipped = reading::addEllipsis(clipped, 17);
                }
                fl_draw(clipped.c_str(),
                        cell.X + kCellPadding,
                        cell.Y + 18 + (lineIndex * lineHeight),
                        cell.W - (kCellPadding * 2),
                        lineHeight,
                        FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_CLIP);
                ++lineIndex;
            }
        }
    }

    fl_pop_clip();
}

MonthCalendarWidget::Cell MonthCalendarWidget::cellAt(int mouseX, int mouseY) const {
    Cell cells[42];
    buildCells(cells);
    for (const Cell& cell : cells) {
        if (mouseX >= cell.X && mouseX < cell.X + cell.W &&
            mouseY >= cell.Y && mouseY < cell.Y + cell.H) {
            return cell;
        }
    }
    return Cell{};
}

void MonthCalendarWidget::buildCells(Cell cells[42]) const {
    const reading::Date monthStart{displayedMonth_.year, displayedMonth_.month, 1};
    const int offset = reading::weekdaySundayFirst(monthStart);
    const reading::Date firstCellDate = reading::addDays(monthStart, -offset);
    const int cellW = std::max(1, w() / 7);
    const int cellH = std::max(32, (h() - kWeekdayHeaderH) / 6);

    for (int i = 0; i < 42; ++i) {
        int row = i / 7;
        int col = i % 7;
        reading::Date cellDate = reading::addDays(firstCellDate, i);
        cells[i].date = cellDate;
        cells[i].inDisplayedMonth = (cellDate.year == displayedMonth_.year &&
                                     cellDate.month == displayedMonth_.month);
        cells[i].X = x() + (col * cellW);
        cells[i].Y = y() + kWeekdayHeaderH + (row * cellH);
        cells[i].W = cellW;
        cells[i].H = cellH;
    }
}

void MonthCalendarWidget::setSelectionRange(const reading::Date& from, const reading::Date& to) {
    if (!from.valid() || !to.valid()) return;

    reading::Date start = from;
    reading::Date end = to;
    if (reading::compareDates(start, end) > 0) {
        std::swap(start, end);
    }

    const std::string startIso = reading::formatIsoDate(start);
    const std::string endIso = reading::formatIsoDate(end);
    if (!selectedDateIsos_.empty() &&
        selectedDateIsos_.front() == startIso &&
        selectedDateIsos_.back() == endIso) {
        return;
    }

    selectedDateIsos_.clear();
    selectedDateSet_.clear();
    reading::Date current = start;
    while (reading::compareDates(current, end) <= 0) {
        std::string currentIso = reading::formatIsoDate(current);
        selectedDateIsos_.push_back(currentIso);
        selectedDateSet_.insert(std::move(currentIso));
        current = reading::addDays(current, 1);
    }
    redraw();
}

} // namespace verdad
