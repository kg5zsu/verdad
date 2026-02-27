#include "ui/VerseContext.h"
#include "app/VerdadApp.h"
#include "ui/MainWindow.h"
#include "ui/BiblePane.h"
#include "ui/LeftPane.h"
#include "ui/TagPanel.h"
#include "sword/SwordManager.h"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/fl_ask.H>

#include <cstring>
#include <cctype>
#include <regex>

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

std::string extractFirstStrongsToken(const std::string& text) {
    static const std::regex kToken(R"(([HhGg]?\d+[A-Za-z]?))");
    std::smatch m;
    if (!std::regex_search(text, m, kToken)) return "";

    std::string tok = trimCopy(m[1].str());
    if (tok.empty()) return "";
    for (char& c : tok) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    return tok;
}

} // namespace

VerseContext::VerseContext(VerdadApp* app)
    : app_(app) {}

VerseContext::~VerseContext() = default;

void VerseContext::show(const std::string& word, const std::string& href,
                         const std::string& strong, const std::string& morph,
                         const std::string& verseKey,
                         int screenX, int screenY) {
    currentWord_ = word;
    currentHref_ = href;
    currentStrong_ = strong;
    currentMorph_ = morph;
    currentStrongsNumber_.clear();
    currentVerseKey_ = verseKey;

    Fl_Menu_Button menu(screenX, screenY, 0, 0);

    // Always available options
    menu.add("Copy Verse Reference", 0, onCopyVerse, this);

    if (!word.empty()) {
        menu.add("Copy Word", 0, onCopyWord, this);
    }

    // Strong's number search (if href contains strongs info)
    std::string strongsNum = extractStrongsNumber(href, strong);
    if (!strongsNum.empty()) {
        currentStrongsNumber_ = strongsNum;
        std::string label = "Search Strong's: " + strongsNum;
        menu.add(label.c_str(), 0, onSearchStrongs, this);

        std::string dictLabel = "Look up in Dictionary: " + strongsNum;
        menu.add(dictLabel.c_str(), 0, onLookupDictionary, this);
    } else if (!word.empty()) {
        // Try to get Strong's from word info
        std::string currentModule = "";
        if (app_->mainWindow() && app_->mainWindow()->biblePane()) {
            currentModule = app_->mainWindow()->biblePane()->currentModule();
        }

        if (!currentModule.empty()) {
            WordInfo info = app_->swordManager().getWordInfo(
                currentModule, verseKey, word);
            if (!info.strongsNumber.empty()) {
                currentStrongsNumber_ = info.strongsNumber;
                std::string label = "Search Strong's: " + info.strongsNumber;
                menu.add(label.c_str(), 0, onSearchStrongs, this);
                // Store for callback
                currentHref_ = "strongs:" + info.strongsNumber;

                std::string dictLabel = "Look up in Dictionary: " + info.strongsNumber;
                menu.add(dictLabel.c_str(), 0, onLookupDictionary, this);
            }
        }
    }

    // Tag options
    menu.add("Add Tag...", 0, onAddTag, this);

    menu.popup();
}

std::string VerseContext::extractStrongsNumber(const std::string& href,
                                               const std::string& strong) const {
    std::string tok = extractFirstStrongsToken(strong);
    if (!tok.empty()) return tok;

    // Extract from various formats:
    // "strongs:H1234", "strong:G5678", "strongs://H1234"
    size_t pos = href.find("strongs:");
    if (pos == std::string::npos) pos = href.find("strong:");
    if (pos == std::string::npos) return "";

    size_t colonPos = href.find(':', pos);
    if (colonPos == std::string::npos) return "";

    std::string num = href.substr(colonPos + 1);

    // Remove leading slashes
    while (!num.empty() && num[0] == '/') {
        num = num.substr(1);
    }

    return extractFirstStrongsToken(num);
}

void VerseContext::onSearchStrongs(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VerseContext*>(data);

    std::string strongsNum = self->currentStrongsNumber_;
    if (strongsNum.empty()) {
        strongsNum = self->extractStrongsNumber(self->currentHref_, self->currentStrong_);
    }
    if (strongsNum.empty()) return;

    // Execute search and show results
    if (self->app_->mainWindow()) {
        std::string currentModule = "";
        if (self->app_->mainWindow()->biblePane()) {
            currentModule = self->app_->mainWindow()->biblePane()->currentModule();
        }

        if (!currentModule.empty()) {
            self->app_->mainWindow()->showSearchResults(strongsNum);
        }
    }
}

void VerseContext::onAddTag(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VerseContext*>(data);

    if (self->app_->mainWindow() && self->app_->mainWindow()->leftPane()) {
        self->app_->mainWindow()->leftPane()->tagPanel()->showAddTagDialog(
            self->currentVerseKey_);
    }
}

void VerseContext::onCopyVerse(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VerseContext*>(data);

    // Copy verse reference to clipboard
    Fl::copy(self->currentVerseKey_.c_str(),
             static_cast<int>(self->currentVerseKey_.length()), 1);
}

void VerseContext::onCopyWord(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VerseContext*>(data);

    Fl::copy(self->currentWord_.c_str(),
             static_cast<int>(self->currentWord_.length()), 1);
}

void VerseContext::onLookupDictionary(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<VerseContext*>(data);

    std::string strongsNum = self->currentStrongsNumber_;
    if (strongsNum.empty()) {
        strongsNum = self->extractStrongsNumber(self->currentHref_, self->currentStrong_);
    }
    if (strongsNum.empty()) return;

    if (self->app_->mainWindow()) {
        self->app_->mainWindow()->showDictionary(strongsNum);
    }
}

} // namespace verdad
