#include "ui/SearchPanel.h"
#include "app/VerdadApp.h"
#include "ui/MainWindow.h"
#include "ui/LeftPane.h"
#include "sword/SwordManager.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

namespace verdad {

SearchPanel::SearchPanel(VerdadApp* app, int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H)
    , app_(app) {

    begin();

    int padding = 2;
    int choiceH = 25;

    int cy = Y + padding;

    // Module to search
    moduleChoice_ = new Fl_Choice(X + padding, cy, (W - 2 * padding) / 2, choiceH);
    moduleChoice_->tooltip("Module to search in");

    // Search type
    searchType_ = new Fl_Choice(X + padding + (W - 2 * padding) / 2 + 2, cy,
                                 (W - 2 * padding) / 2 - 2, choiceH);
    searchType_->add("Multi-word");
    searchType_->add("Exact phrase");
    searchType_->add("Regex");
    searchType_->value(0);
    searchType_->tooltip("Search type");

    cy += choiceH + padding;

    // Progress bar
    progressBar_ = new Fl_Progress(X + padding, cy, W - 2 * padding, 15);
    progressBar_->minimum(0);
    progressBar_->maximum(1.0);
    progressBar_->hide();

    cy += 17;

    // Result list
    resultBrowser_ = new Fl_Browser(X + padding, cy,
                                     W - 2 * padding, H - (cy - Y) - padding);
    resultBrowser_->type(FL_HOLD_BROWSER);
    resultBrowser_->callback(onResultSelect, this);
    resultBrowser_->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);

    end();
    resizable(resultBrowser_);

    populateModules();
}

SearchPanel::~SearchPanel() = default;

void SearchPanel::search(const std::string& query) {
    results_.clear();
    resultBrowser_->clear();

    // Get selected module
    const Fl_Menu_Item* item = moduleChoice_->mvalue();
    if (!item || !item->label()) {
        fl_alert("Please select a module to search.");
        return;
    }
    std::string moduleName = item->label();

    // Get search type
    int type = -1; // multi-word
    switch (searchType_->value()) {
    case 0: type = -1; break; // multi-word
    case 1: type = 1; break;  // phrase
    case 2: type = 0; break;  // regex
    }

    // Show progress bar
    progressBar_->show();
    progressBar_->value(0);
    Fl::flush();

    // Execute search
    results_ = app_->swordManager().search(
        moduleName, query, type, "",
        [this](float progress) {
            progressBar_->value(progress);
            Fl::flush();
        });

    progressBar_->hide();

    // Populate result browser
    for (const auto& r : results_) {
        std::string line = r.key + " - " + r.text.substr(0, 60);
        resultBrowser_->add(line.c_str());
    }

    // Update browser label
    std::string label = "Results: " + std::to_string(results_.size());
    resultBrowser_->copy_label(label.c_str());

    redraw();
}

void SearchPanel::clear() {
    results_.clear();
    resultBrowser_->clear();
}

const SearchResult* SearchPanel::selectedResult() const {
    int idx = resultBrowser_->value();
    if (idx > 0 && idx <= static_cast<int>(results_.size())) {
        return &results_[idx - 1]; // Browser is 1-indexed
    }
    return nullptr;
}

void SearchPanel::populateModules() {
    if (!moduleChoice_) return;
    moduleChoice_->clear();

    // Add Bible modules first
    auto bibles = app_->swordManager().getBibleModules();
    for (const auto& mod : bibles) {
        moduleChoice_->add(mod.name.c_str());
    }

    // Then commentaries
    auto commentaries = app_->swordManager().getCommentaryModules();
    for (const auto& mod : commentaries) {
        moduleChoice_->add(mod.name.c_str());
    }

    if (moduleChoice_->size() > 0) {
        moduleChoice_->value(0);
    }
}

void SearchPanel::onResultSelect(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<SearchPanel*>(data);

    const SearchResult* result = self->selectedResult();
    if (!result) return;

    // Update preview in left pane
    if (self->app_->mainWindow() && self->app_->mainWindow()->leftPane()) {
        std::string html = self->app_->swordManager().getVerseText(
            result->module, result->key);
        self->app_->mainWindow()->leftPane()->setPreviewText(html);
    }

    // On double-click, navigate to the verse
    if (Fl::event_clicks() > 0) {
        onResultDoubleClick(nullptr, data);
    }
}

void SearchPanel::onResultDoubleClick(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<SearchPanel*>(data);

    const SearchResult* result = self->selectedResult();
    if (!result) return;

    // Navigate to the verse
    if (self->app_->mainWindow()) {
        self->app_->mainWindow()->navigateTo(result->module, result->key);
    }
}

} // namespace verdad
