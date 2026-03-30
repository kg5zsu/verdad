#include "ui/ReadingPlanEditorDialog.h"

#include "reading/DateUtils.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Multiline_Input.H>
#include <FL/Fl_Return_Button.H>
#include <FL/fl_ask.H>

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        line = trimCopy(line);
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

std::string joinPassages(const std::vector<ReadingPlanPassage>& passages,
                         const char* separator) {
    std::ostringstream out;
    for (size_t i = 0; i < passages.size(); ++i) {
        if (i) out << separator;
        out << passages[i].reference;
    }
    return out.str();
}

std::string formatDayLabel(const ReadingPlanDay& day) {
    std::ostringstream label;
    label << day.dateIso;
    if (!trimCopy(day.title).empty()) {
        label << "  |  " << day.title;
    }
    if (!day.passages.empty()) {
        label << "  |  " << joinPassages(day.passages, "; ");
    } else {
        label << "  |  (No passages)";
    }
    return label.str();
}

bool compareDaysByDate(const ReadingPlanDay& lhs, const ReadingPlanDay& rhs) {
    if (lhs.dateIso != rhs.dateIso) return lhs.dateIso < rhs.dateIso;
    if (lhs.title != rhs.title) return lhs.title < rhs.title;
    return joinPassages(lhs.passages, ";") < joinPassages(rhs.passages, ";");
}

void normalizePlanDays(std::vector<ReadingPlanDay>& days) {
    for (auto& day : days) {
        day.dateIso = trimCopy(day.dateIso);
        day.title = trimCopy(day.title);

        std::vector<ReadingPlanPassage> cleaned;
        for (auto passage : day.passages) {
            passage.reference = trimCopy(passage.reference);
            if (!passage.reference.empty()) {
                passage.id = 0;
                cleaned.push_back(std::move(passage));
            }
        }
        day.passages = std::move(cleaned);
        day.id = 0;
    }

    days.erase(std::remove_if(days.begin(), days.end(),
                              [](const ReadingPlanDay& day) {
                                  return day.dateIso.empty();
                              }),
               days.end());
    std::sort(days.begin(), days.end(), compareDaysByDate);

    std::vector<ReadingPlanDay> merged;
    for (auto& day : days) {
        if (!merged.empty() && merged.back().dateIso == day.dateIso) {
            if (merged.back().title.empty()) merged.back().title = day.title;
            merged.back().completed = merged.back().completed || day.completed;
            merged.back().passages.insert(merged.back().passages.end(),
                                          day.passages.begin(),
                                          day.passages.end());
        } else {
            merged.push_back(std::move(day));
        }
    }
    days = std::move(merged);
}

bool editPlanDay(ReadingPlanDay& day, bool creating) {
    const int dialogW = 560;
    const int dialogH = 360;
    Fl_Double_Window dialog(dialogW, dialogH,
                            creating ? "Add Reading Day" : "Edit Reading Day");
    dialog.set_modal();

    Fl_Box dateLabel(16, 18, 96, 24, "Date");
    dateLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input dateInput(120, 16, dialogW - 136, 26);
    dateInput.value(day.dateIso.c_str());
    dateInput.tooltip("YYYY-MM-DD");

    Fl_Box titleLabel(16, 54, 96, 24, "Title");
    titleLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input titleInput(120, 52, dialogW - 136, 26);
    titleInput.value(day.title.c_str());

    Fl_Box passagesLabel(16, 92, 160, 24, "Passages");
    passagesLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Multiline_Input passagesInput(16, 118, dialogW - 32, 176);
    passagesInput.tooltip("Enter one Bible reference per line");
    passagesInput.value(joinPassages(day.passages, "\n").c_str());

    Fl_Button cancelButton(dialogW - 184, dialogH - 42, 76, 28, "Cancel");
    Fl_Return_Button okButton(dialogW - 96, dialogH - 42, 76, 28, "OK");

    bool accepted = false;
    cancelButton.callback([](Fl_Widget*, void* data) {
        static_cast<Fl_Window*>(data)->hide();
    }, &dialog);
    okButton.callback([](Fl_Widget*, void* data) {
        bool* acceptedPtr = static_cast<bool*>(data);
        *acceptedPtr = true;
    }, &accepted);

    dialog.end();
    dialog.hotspot(&okButton);
    dialog.show();

    while (dialog.shown()) {
        Fl::wait();
        if (!accepted) continue;

        ReadingPlanDay updated = day;
        updated.dateIso = trimCopy(dateInput.value() ? dateInput.value() : "");
        updated.title = trimCopy(titleInput.value() ? titleInput.value() : "");
        updated.passages.clear();
        for (const auto& ref : splitLines(passagesInput.value() ? passagesInput.value() : "")) {
            updated.passages.push_back(ReadingPlanPassage{0, ref});
        }

        if (!reading::isIsoDateInRange(updated.dateIso)) {
            fl_alert("Enter a valid date in YYYY-MM-DD format.");
            accepted = false;
            continue;
        }
        if (updated.passages.empty()) {
            fl_alert("Add at least one passage for this day.");
            accepted = false;
            continue;
        }

        day = std::move(updated);
        dialog.hide();
    }

    return accepted;
}

bool promptShiftAllDays(std::vector<ReadingPlanDay>& days) {
    Fl_Double_Window dialog(320, 138, "Bulk Shift Dates");
    dialog.set_modal();

    Fl_Box help(16, 16, 288, 30, "Shift every reading day by this many days.");
    help.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    Fl_Box deltaLabel(16, 58, 70, 24, "Delta");
    deltaLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input deltaInput(88, 56, 80, 26);
    deltaInput.value("1");

    Fl_Button cancelButton(144, 96, 76, 28, "Cancel");
    Fl_Return_Button okButton(228, 96, 76, 28, "OK");

    bool accepted = false;
    cancelButton.callback([](Fl_Widget*, void* data) {
        static_cast<Fl_Window*>(data)->hide();
    }, &dialog);
    okButton.callback([](Fl_Widget*, void* data) {
        *static_cast<bool*>(data) = true;
    }, &accepted);

    dialog.end();
    dialog.hotspot(&okButton);
    dialog.show();

    while (dialog.shown()) {
        Fl::wait();
        if (!accepted) continue;

        int delta = 0;
        try {
            delta = std::stoi(trimCopy(deltaInput.value() ? deltaInput.value() : "0"));
        } catch (...) {
            fl_alert("Enter a whole number of days.");
            accepted = false;
            continue;
        }

        for (auto& day : days) {
            reading::Date current{};
            if (!reading::parseIsoDate(day.dateIso, current)) continue;
            day.dateIso = reading::formatIsoDate(reading::addDays(current, delta));
        }
        normalizePlanDays(days);
        dialog.hide();
    }

    return accepted;
}

bool promptGenerateStarter(std::vector<ReadingPlanDay>& days,
                           std::string& startDateIso) {
    const int dialogW = 560;
    const int dialogH = 360;
    Fl_Double_Window dialog(dialogW, dialogH, "Generate Starter Schedule");
    dialog.set_modal();

    Fl_Box startLabel(16, 16, 96, 24, "Start date");
    startLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input startInput(116, 14, 120, 26);
    if (reading::isIsoDateInRange(startDateIso)) {
        startInput.value(startDateIso.c_str());
    } else {
        startInput.value(reading::formatIsoDate(reading::today()).c_str());
    }

    Fl_Box prefixLabel(250, 16, 96, 24, "Title prefix");
    prefixLabel.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    Fl_Input prefixInput(342, 14, dialogW - 358, 26);
    prefixInput.value("Day");

    Fl_Box help(16, 54, dialogW - 32, 40,
                "Enter one passage per line. The helper will create explicit dated days "
                "that you can edit afterwards.");
    help.align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    Fl_Multiline_Input passagesInput(16, 98, dialogW - 32, 212);
    passagesInput.tooltip("One reference per line");

    Fl_Button cancelButton(dialogW - 184, dialogH - 42, 76, 28, "Cancel");
    Fl_Return_Button okButton(dialogW - 96, dialogH - 42, 76, 28, "OK");

    bool accepted = false;
    cancelButton.callback([](Fl_Widget*, void* data) {
        static_cast<Fl_Window*>(data)->hide();
    }, &dialog);
    okButton.callback([](Fl_Widget*, void* data) {
        *static_cast<bool*>(data) = true;
    }, &accepted);

    dialog.end();
    dialog.hotspot(&okButton);
    dialog.show();

    while (dialog.shown()) {
        Fl::wait();
        if (!accepted) continue;

        std::string generatedStart = trimCopy(startInput.value() ? startInput.value() : "");
        if (!reading::isIsoDateInRange(generatedStart)) {
            fl_alert("Enter a valid start date in YYYY-MM-DD format.");
            accepted = false;
            continue;
        }

        std::vector<std::string> passages = splitLines(
            passagesInput.value() ? passagesInput.value() : "");
        if (passages.empty()) {
            fl_alert("Enter at least one passage.");
            accepted = false;
            continue;
        }

        int replaceChoice = 1;
        if (!days.empty()) {
            replaceChoice = fl_choice("Replace the current days or append the generated starter schedule?",
                                      "Cancel",
                                      "Replace",
                                      "Append");
            if (replaceChoice == 0) {
                accepted = false;
                continue;
            }
        }

        std::vector<ReadingPlanDay> generated;
        reading::Date startDate{};
        reading::parseIsoDate(generatedStart, startDate);
        const std::string prefix = trimCopy(prefixInput.value() ? prefixInput.value() : "");
        for (size_t i = 0; i < passages.size(); ++i) {
            ReadingPlanDay day;
            day.dateIso = reading::formatIsoDate(
                reading::addDays(startDate, static_cast<int>(i)));
            if (!prefix.empty()) {
                day.title = prefix + " " + std::to_string(i + 1);
            }
            day.passages.push_back(ReadingPlanPassage{0, passages[i]});
            generated.push_back(std::move(day));
        }

        if (replaceChoice == 1 || days.empty()) {
            days = std::move(generated);
        } else {
            days.insert(days.end(), generated.begin(), generated.end());
        }
        normalizePlanDays(days);
        startDateIso = generatedStart;
        dialog.hide();
    }

    return accepted;
}

class ReadingPlanEditorController {
public:
    ReadingPlanEditorController(ReadingPlan& plan, bool creating)
        : dialog_(900, 620, creating ? "New Reading Plan" : "Edit Reading Plan")
        , workingPlan_(plan)
        , creating_(creating) {
        dialog_.set_modal();
        buildUi();
        loadFromPlan();
    }

    bool open() {
        dialog_.show();
        while (dialog_.shown()) {
            Fl::wait();
        }
        return accepted_;
    }

    const ReadingPlan& plan() const { return workingPlan_; }

private:
    Fl_Double_Window dialog_;
    ReadingPlan workingPlan_;
    bool creating_ = false;
    bool accepted_ = false;

    Fl_Input* nameInput_ = nullptr;
    Fl_Multiline_Input* descriptionInput_ = nullptr;
    Fl_Input* colorInput_ = nullptr;
    Fl_Input* startDateInput_ = nullptr;
    Fl_Hold_Browser* dayBrowser_ = nullptr;
    Fl_Box* summaryBox_ = nullptr;
    Fl_Button* editDayButton_ = nullptr;
    Fl_Button* duplicateDayButton_ = nullptr;
    Fl_Button* removeDayButton_ = nullptr;
    Fl_Button* moveEarlierButton_ = nullptr;
    Fl_Button* moveLaterButton_ = nullptr;

    void buildUi() {
        const int margin = 14;
        const int labelW = 82;

        Fl_Box* nameLabel = new Fl_Box(margin, 16, labelW, 24, "Name");
        nameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        nameInput_ = new Fl_Input(margin + labelW, 14, 320, 26);

        Fl_Box* colorLabel = new Fl_Box(430, 16, 54, 24, "Color");
        colorLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        colorInput_ = new Fl_Input(486, 14, 100, 26);
        colorInput_->tooltip("Optional hex color, for example #5d8aa8");

        Fl_Box* startLabel = new Fl_Box(604, 16, 70, 24, "Start");
        startLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        startDateInput_ = new Fl_Input(676, 14, 120, 26);
        startDateInput_->tooltip("YYYY-MM-DD");

        Fl_Box* descriptionLabel = new Fl_Box(margin, 52, labelW, 24, "Notes");
        descriptionLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
        descriptionInput_ = new Fl_Multiline_Input(margin + labelW, 50, 700, 64);

        dayBrowser_ = new Fl_Hold_Browser(margin, 128, 610, 410);
        dayBrowser_->callback(onDaySelected, this);

        Fl_Button* addDayButton = new Fl_Button(640, 128, 120, 28, "Add Day");
        addDayButton->callback(onAddDay, this);
        editDayButton_ = new Fl_Button(640, 164, 120, 28, "Edit Day");
        editDayButton_->callback(onEditDay, this);
        duplicateDayButton_ = new Fl_Button(640, 200, 120, 28, "Duplicate");
        duplicateDayButton_->callback(onDuplicateDay, this);
        removeDayButton_ = new Fl_Button(640, 236, 120, 28, "Remove");
        removeDayButton_->callback(onRemoveDay, this);
        moveEarlierButton_ = new Fl_Button(640, 284, 120, 28, "Move Earlier");
        moveEarlierButton_->callback(onMoveEarlier, this);
        moveLaterButton_ = new Fl_Button(640, 320, 120, 28, "Move Later");
        moveLaterButton_->callback(onMoveLater, this);

        Fl_Button* shiftButton = new Fl_Button(640, 372, 120, 28, "Bulk Shift");
        shiftButton->callback(onBulkShift, this);
        Fl_Button* generateButton = new Fl_Button(640, 408, 180, 28, "Generate Starter...");
        generateButton->callback(onGenerateStarter, this);

        summaryBox_ = new Fl_Box(640, 456, 230, 82, "");
        summaryBox_->box(FL_DOWN_FRAME);
        summaryBox_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

        Fl_Button* cancelButton = new Fl_Button(dialog_.w() - 184, dialog_.h() - 42, 76, 28, "Cancel");
        auto* okButton = new Fl_Return_Button(dialog_.w() - 96, dialog_.h() - 42, 76, 28, "OK");
        cancelButton->callback(onCancel, this);
        okButton->callback(onAccept, this);

        dialog_.end();
        dialog_.hotspot(okButton);
    }

    void loadFromPlan() {
        nameInput_->value(workingPlan_.summary.name.c_str());
        descriptionInput_->value(workingPlan_.summary.description.c_str());
        colorInput_->value(workingPlan_.summary.color.c_str());
        if (reading::isIsoDateInRange(workingPlan_.summary.startDateIso)) {
            startDateInput_->value(workingPlan_.summary.startDateIso.c_str());
        } else {
            startDateInput_->value(reading::formatIsoDate(reading::today()).c_str());
        }

        normalizePlanDays(workingPlan_.days);
        rebuildDayBrowser();
        updateDayButtons();
    }

    void rebuildDayBrowser() {
        if (!dayBrowser_) return;
        const int previous = dayBrowser_->value();
        dayBrowser_->clear();
        for (const auto& day : workingPlan_.days) {
            dayBrowser_->add(formatDayLabel(day).c_str());
        }
        if (!workingPlan_.days.empty()) {
            int target = previous;
            if (target <= 0 || target > dayBrowser_->size()) {
                target = 1;
            }
            dayBrowser_->select(target);
        }
        updateSummary();
    }

    int selectedDayIndex() const {
        if (!dayBrowser_) return -1;
        int line = dayBrowser_->value();
        if (line <= 0 || line > static_cast<int>(workingPlan_.days.size())) return -1;
        return line - 1;
    }

    void updateSummary() {
        if (!summaryBox_) return;

        size_t totalPassages = 0;
        for (const auto& day : workingPlan_.days) {
            totalPassages += day.passages.size();
        }

        std::ostringstream summary;
        summary << "Days: " << workingPlan_.days.size()
                << "\nPassages: " << totalPassages;
        if (!workingPlan_.days.empty()) {
            summary << "\nFirst: " << workingPlan_.days.front().dateIso
                    << "\nLast: " << workingPlan_.days.back().dateIso;
        }
        summaryBox_->copy_label(summary.str().c_str());
    }

    void updateDayButtons() {
        const int index = selectedDayIndex();
        const bool hasSelection = index >= 0;
        if (editDayButton_) hasSelection ? editDayButton_->activate() : editDayButton_->deactivate();
        if (duplicateDayButton_) hasSelection ? duplicateDayButton_->activate() : duplicateDayButton_->deactivate();
        if (removeDayButton_) hasSelection ? removeDayButton_->activate() : removeDayButton_->deactivate();
        if (moveEarlierButton_) {
            if (index > 0) moveEarlierButton_->activate();
            else moveEarlierButton_->deactivate();
        }
        if (moveLaterButton_) {
            if (index >= 0 && index + 1 < static_cast<int>(workingPlan_.days.size())) {
                moveLaterButton_->activate();
            } else {
                moveLaterButton_->deactivate();
            }
        }
    }

    void addDay() {
        ReadingPlanDay day;
        if (!workingPlan_.days.empty()) {
            reading::Date last{};
            if (reading::parseIsoDate(workingPlan_.days.back().dateIso, last)) {
                day.dateIso = reading::formatIsoDate(reading::addDays(last, 1));
            }
        }
        if (day.dateIso.empty()) {
            std::string start = trimCopy(startDateInput_->value() ? startDateInput_->value() : "");
            if (reading::isIsoDateInRange(start)) {
                day.dateIso = start;
            } else {
                day.dateIso = reading::formatIsoDate(reading::today());
            }
        }
        if (!editPlanDay(day, true)) return;
        workingPlan_.days.push_back(std::move(day));
        normalizePlanDays(workingPlan_.days);
        rebuildDayBrowser();
        updateDayButtons();
    }

    void editSelectedDay() {
        int index = selectedDayIndex();
        if (index < 0) return;
        ReadingPlanDay edited = workingPlan_.days[static_cast<size_t>(index)];
        if (!editPlanDay(edited, false)) return;
        workingPlan_.days[static_cast<size_t>(index)] = std::move(edited);
        normalizePlanDays(workingPlan_.days);
        rebuildDayBrowser();
        updateDayButtons();
    }

    void duplicateSelectedDay() {
        int index = selectedDayIndex();
        if (index < 0) return;
        ReadingPlanDay copy = workingPlan_.days[static_cast<size_t>(index)];
        copy.id = 0;
        for (auto& passage : copy.passages) {
            passage.id = 0;
        }
        reading::Date date{};
        if (reading::parseIsoDate(copy.dateIso, date)) {
            copy.dateIso = reading::formatIsoDate(reading::addDays(date, 1));
        }
        workingPlan_.days.push_back(std::move(copy));
        normalizePlanDays(workingPlan_.days);
        rebuildDayBrowser();
        updateDayButtons();
    }

    void removeSelectedDay() {
        int index = selectedDayIndex();
        if (index < 0) return;
        workingPlan_.days.erase(workingPlan_.days.begin() + index);
        rebuildDayBrowser();
        updateDayButtons();
    }

    void moveSelected(int delta) {
        int index = selectedDayIndex();
        int other = index + delta;
        if (index < 0 || other < 0 ||
            other >= static_cast<int>(workingPlan_.days.size())) {
            return;
        }

        std::swap(workingPlan_.days[static_cast<size_t>(index)].dateIso,
                  workingPlan_.days[static_cast<size_t>(other)].dateIso);
        normalizePlanDays(workingPlan_.days);
        rebuildDayBrowser();
        if (dayBrowser_ && other >= 0 && other < dayBrowser_->size()) {
            dayBrowser_->select(other + 1);
        }
        updateDayButtons();
    }

    bool validateAndStore() {
        ReadingPlan updated = workingPlan_;
        updated.summary.name = trimCopy(nameInput_->value() ? nameInput_->value() : "");
        updated.summary.description =
            trimCopy(descriptionInput_->value() ? descriptionInput_->value() : "");
        updated.summary.color = trimCopy(colorInput_->value() ? colorInput_->value() : "");
        updated.summary.startDateIso =
            trimCopy(startDateInput_->value() ? startDateInput_->value() : "");

        if (updated.summary.name.empty()) {
            fl_alert("Enter a plan name.");
            return false;
        }
        if (!updated.summary.startDateIso.empty() &&
            !reading::isIsoDateInRange(updated.summary.startDateIso)) {
            fl_alert("Enter a valid start date in YYYY-MM-DD format.");
            return false;
        }
        if (updated.days.empty()) {
            fl_alert("Add at least one reading day.");
            return false;
        }

        normalizePlanDays(updated.days);
        for (const auto& day : updated.days) {
            if (!reading::isIsoDateInRange(day.dateIso)) {
                fl_alert("One of the reading days has an invalid date.");
                return false;
            }
            if (day.passages.empty()) {
                fl_alert("Every reading day needs at least one passage.");
                return false;
            }
        }

        workingPlan_ = std::move(updated);
        workingPlan_.summary.totalDays = static_cast<int>(workingPlan_.days.size());
        workingPlan_.summary.completedDays = 0;
        for (const auto& day : workingPlan_.days) {
            if (day.completed) ++workingPlan_.summary.completedDays;
        }
        return true;
    }

    static void onDaySelected(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->updateDayButtons();
    }

    static void onAddDay(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->addDay();
    }

    static void onEditDay(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->editSelectedDay();
    }

    static void onDuplicateDay(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->duplicateSelectedDay();
    }

    static void onRemoveDay(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->removeSelectedDay();
    }

    static void onMoveEarlier(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->moveSelected(-1);
    }

    static void onMoveLater(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->moveSelected(1);
    }

    static void onBulkShift(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        if (promptShiftAllDays(self->workingPlan_.days)) {
            self->rebuildDayBrowser();
            self->updateDayButtons();
        }
    }

    static void onGenerateStarter(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        std::string startDate =
            trimCopy(self->startDateInput_->value() ? self->startDateInput_->value() : "");
        if (promptGenerateStarter(self->workingPlan_.days, startDate)) {
            self->startDateInput_->value(startDate.c_str());
            self->rebuildDayBrowser();
            self->updateDayButtons();
        }
    }

    static void onCancel(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        self->dialog_.hide();
    }

    static void onAccept(Fl_Widget*, void* data) {
        auto* self = static_cast<ReadingPlanEditorController*>(data);
        if (!self) return;
        if (!self->validateAndStore()) return;
        self->accepted_ = true;
        self->dialog_.hide();
    }
};

} // namespace

bool ReadingPlanEditorDialog::editPlan(ReadingPlan& plan, bool creating) {
    ReadingPlanEditorController controller(plan, creating);
    if (!controller.open()) return false;
    plan = controller.plan();
    return true;
}

} // namespace verdad
