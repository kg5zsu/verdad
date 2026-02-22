#include "ui/MainWindow.h"
#include "app/VerdadApp.h"
#include "ui/LeftPane.h"
#include "ui/BiblePane.h"
#include "ui/RightPane.h"
#include "ui/ToolTipWindow.h"
#include "sword/SwordManager.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>

#include <sstream>

namespace verdad {

MainWindow::MainWindow(VerdadApp* app, int W, int H, const char* title)
    : Fl_Double_Window(W, H, title)
    , app_(app)
    , tooltipWindow_(nullptr) {

    // Set minimum size
    size_range(800, 600);

    int menuH = 25;

    // Menu bar
    menuBar_ = new Fl_Menu_Bar(0, 0, W, menuH);
    buildMenu();

    // Three-pane layout using Fl_Tile
    mainTile_ = new Fl_Tile(0, menuH, W, H - menuH);
    mainTile_->begin();

    // Left pane (about 25% width)
    int leftW = W * 25 / 100;
    leftPane_ = new LeftPane(app_, 0, menuH, leftW, H - menuH);

    // Bible pane (about 50% width)
    int bibleW = W * 50 / 100;
    biblePane_ = new BiblePane(app_, leftW, menuH, bibleW, H - menuH);

    // Right pane (remaining width)
    int rightX = leftW + bibleW;
    int rightW = W - rightX;
    rightPane_ = new RightPane(app_, rightX, menuH, rightW, H - menuH);

    mainTile_->end();

    // Create tooltip window (floating, not part of layout)
    tooltipWindow_ = new ToolTipWindow();

    // Set resizable
    resizable(mainTile_);

    end();
}

MainWindow::~MainWindow() {
    delete tooltipWindow_;
}

void MainWindow::navigateTo(const std::string& reference) {
    if (biblePane_) {
        biblePane_->navigateToReference(reference);
    }
}

void MainWindow::navigateTo(const std::string& module, const std::string& reference) {
    if (biblePane_) {
        biblePane_->setModule(module);
        biblePane_->navigateToReference(reference);
    }
}

void MainWindow::showCommentary(const std::string& reference) {
    if (rightPane_) {
        rightPane_->showCommentary(reference);
    }
}

void MainWindow::showDictionary(const std::string& key) {
    if (rightPane_) {
        rightPane_->showDictionaryEntry(key);
    }
}

void MainWindow::showWordInfo(const std::string& word, const std::string& href,
                               int screenX, int screenY) {
    if (!tooltipWindow_) return;

    // Get word info from SWORD
    std::string currentModule = biblePane_ ? biblePane_->currentModule() : "";
    std::string currentRef = "";
    if (biblePane_) {
        currentRef = biblePane_->currentBook() + " "
                     + std::to_string(biblePane_->currentChapter());
    }

    // Build tooltip HTML
    std::ostringstream html;
    html << "<div class=\"tooltip\">";

    // Try to extract Strong's number from href
    std::string strongsNum;
    if (href.find("strongs:") == 0) {
        strongsNum = href.substr(8);
    } else if (href.find("strong:") == 0) {
        strongsNum = href.substr(7);
    }

    if (!strongsNum.empty()) {
        html << "<span class=\"strongs-label\">Strong's: " << strongsNum << "</span><br/>";

        std::string def = app_->swordManager().getStrongsDefinition(strongsNum);
        if (!def.empty()) {
            html << "<span class=\"definition\">" << def << "</span><br/>";
        }
    }

    if (!word.empty()) {
        // Try to get word info
        WordInfo info = app_->swordManager().getWordInfo(
            currentModule, currentRef, word);

        if (!info.strongsNumber.empty() && strongsNum.empty()) {
            html << "<span class=\"strongs-label\">Strong's: "
                 << info.strongsNumber << "</span><br/>";
            if (!info.strongsDef.empty()) {
                html << "<span class=\"definition\">"
                     << info.strongsDef << "</span><br/>";
            }
        }

        if (!info.morphCode.empty()) {
            html << "<span class=\"morph-label\">Morph: "
                 << info.morphCode << "</span><br/>";
            if (!info.morphDef.empty()) {
                html << "<span class=\"definition\">"
                     << info.morphDef << "</span>";
            }
        }
    }

    html << "</div>";

    std::string tooltipHtml = html.str();
    if (tooltipHtml.find("class=\"definition\"") != std::string::npos ||
        tooltipHtml.find("class=\"strongs-label\"") != std::string::npos) {
        tooltipWindow_->showAt(screenX + 15, screenY + 15, tooltipHtml);
    }
}

void MainWindow::hideWordInfo() {
    if (tooltipWindow_) {
        tooltipWindow_->hideTooltip();
    }
}

void MainWindow::showSearchResults(const std::string& query) {
    if (leftPane_) {
        leftPane_->doSearch(query);
        leftPane_->showSearchTab();
    }
}

void MainWindow::refresh() {
    if (leftPane_) leftPane_->refresh();
    if (biblePane_) biblePane_->refresh();
    if (rightPane_) rightPane_->refresh();
}

int MainWindow::handle(int event) {
    // Global keyboard shortcuts
    if (event == FL_SHORTCUT) {
        if (Fl::event_key() == FL_Escape) {
            hideWordInfo();
            return 1;
        }
    }
    return Fl_Double_Window::handle(event);
}

void MainWindow::buildMenu() {
    menuBar_->add("&File/&Quit", FL_CTRL + 'q', onFileQuit, this);
    menuBar_->add("&Navigate/&Go to Verse...", FL_CTRL + 'g', onNavigateGo, this);
    menuBar_->add("&View/&Parallel Bibles", FL_CTRL + 'p', onViewParallel, this);
    menuBar_->add("&Help/&About Verdad", 0, onHelpAbout, this);
}

void MainWindow::onFileQuit(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    self->hide();
}

void MainWindow::onNavigateGo(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    const char* ref = fl_input("Go to verse:", "Genesis 1:1");
    if (ref && ref[0]) {
        self->navigateTo(ref);
    }
}

void MainWindow::onViewParallel(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<MainWindow*>(data);
    if (self->biblePane_) {
        self->biblePane_->toggleParallel();
    }
}

void MainWindow::onHelpAbout(Fl_Widget* /*w*/, void* /*data*/) {
    fl_message("Verdad Bible Study\n\n"
               "A Bible study application using:\n"
               "- FLTK for the user interface\n"
               "- litehtml for XHTML rendering\n"
               "- SWORD library for Bible modules\n\n"
               "Version 0.1.0");
}

} // namespace verdad
