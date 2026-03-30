#include "ui/MonthCalendarWidget.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <algorithm>

namespace verdad {
namespace {

constexpr int kWeekdayHeaderH = 20;
constexpr int kCellPadding = 4;

const char* kWeekdayLabels[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

Fl_Color summaryColorForCell(const CalendarDayMeta& meta, bool inDisplayedMonth) {
    if (!inDisplayedMonth) return fl_rgb_color(150, 150, 150);
    if (meta.completed) return fl_rgb_color(16, 120, 52);
    if (meta.overdue) return fl_rgb_color(180, 80, 40);
    return fl_rgb_color(70, 70, 70);
}

} // namespace

MonthCalendarWidget::MonthCalendarWidget(int X, int Y, int W, int H, const char* label)
    : Fl_Widget(X, Y, W, H, label)
    , displayedMonth_(reading::today())
    , selectedDate_(reading::today()) {
    box(FL_FLAT_BOX);
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
    redraw();
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
            selectedDate_ = cell.date;
            if (selectCallback_) selectCallback_(cell.date);
            redraw();
            return 1;
        }
    }
    return Fl_Widget::handle(event);
}

void MonthCalendarWidget::draw() {
    fl_push_clip(x(), y(), w(), h());
    fl_color(color());
    fl_rectf(x(), y(), w(), h());

    const int cellW = std::max(1, w() / 7);
    const int gridTop = y() + kWeekdayHeaderH;
    const int cellH = std::max(32, (h() - kWeekdayHeaderH) / 6);

    fl_font(labelfont(), 11);
    for (int col = 0; col < 7; ++col) {
        int cellX = x() + (col * cellW);
        fl_color(fl_rgb_color(85, 85, 85));
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

        bool selected = reading::sameDate(cell.date, selectedDate_);
        bool isToday = reading::sameDate(cell.date, today);

        Fl_Color fill = cell.inDisplayedMonth ? fl_rgb_color(250, 250, 250)
                                              : fl_rgb_color(240, 240, 240);
        if (selected) {
            fill = fl_rgb_color(220, 232, 248);
        } else if (meta.completed) {
            fill = fl_rgb_color(232, 247, 236);
        } else if (meta.overdue) {
            fill = fl_rgb_color(252, 238, 232);
        }
        fl_color(fill);
        fl_rectf(cell.X, cell.Y, cell.W, cell.H);

        fl_color(fl_rgb_color(210, 210, 210));
        fl_rect(cell.X, cell.Y, cell.W, cell.H);

        if (isToday) {
            fl_color(fl_rgb_color(46, 111, 191));
            fl_rect(cell.X + 1, cell.Y + 1, cell.W - 2, cell.H - 2);
        }
        if (selected) {
            fl_color(fl_rgb_color(30, 83, 153));
            fl_rect(cell.X + 2, cell.Y + 2, cell.W - 4, cell.H - 4);
        }

        fl_color(cell.inDisplayedMonth ? FL_BLACK : fl_rgb_color(140, 140, 140));
        fl_font(labelfont(), 12);
        fl_draw(std::to_string(cell.date.day).c_str(),
                cell.X + kCellPadding,
                cell.Y + kCellPadding + 10);

        if (meta.completed) {
            fl_color(fl_rgb_color(16, 120, 52));
            fl_draw("@check", cell.X + cell.W - 18, cell.Y + 2, 16, 16, FL_ALIGN_CENTER);
        } else if (meta.overdue) {
            fl_color(fl_rgb_color(180, 80, 40));
            fl_draw("!", cell.X + cell.W - 16, cell.Y + 2, 12, 16, FL_ALIGN_CENTER);
        }

        if (!meta.summary.empty()) {
            fl_color(summaryColorForCell(meta, cell.inDisplayedMonth));
            fl_font(FL_HELVETICA, 10);
            const std::string clipped = reading::addEllipsis(meta.summary, 18);
            fl_draw(clipped.c_str(),
                    cell.X + kCellPadding,
                    cell.Y + 18,
                    cell.W - (kCellPadding * 2),
                    cell.H - 20,
                    FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_CLIP);
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

} // namespace verdad
