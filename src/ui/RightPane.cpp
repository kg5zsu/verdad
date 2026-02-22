#include "ui/RightPane.h"
#include "app/VerdadApp.h"
#include "ui/HtmlWidget.h"
#include "sword/SwordManager.h"

#include <FL/Fl.H>

#include <fstream>

namespace verdad {

RightPane::RightPane(VerdadApp* app, int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H)
    , app_(app) {

    begin();

    int padding = 2;

    // Tabs for commentary and dictionary
    tabs_ = new Fl_Tabs(X + padding, Y + padding,
                         W - 2 * padding, H - 2 * padding);
    tabs_->begin();

    int tabY = Y + padding + 25;
    int tabH = H - 2 * padding - 25;
    int choiceH = 25;

    // --- Commentary tab ---
    commentaryGroup_ = new Fl_Group(X + padding, tabY,
                                     W - 2 * padding, tabH, "Commentary");
    commentaryGroup_->begin();

    commentaryChoice_ = new Fl_Choice(X + padding + 2, tabY + 2,
                                       W - 2 * padding - 4, choiceH);
    commentaryChoice_->callback(onCommentaryModuleChange, this);

    commentaryHtml_ = new HtmlWidget(X + padding + 2, tabY + choiceH + 4,
                                      W - 2 * padding - 4,
                                      tabH - choiceH - 6);

    // Load CSS
    std::string cssFile = app_->getDataDir() + "/master.css";
    std::ifstream cssStream(cssFile);
    if (cssStream.is_open()) {
        std::string css((std::istreambuf_iterator<char>(cssStream)),
                         std::istreambuf_iterator<char>());
        commentaryHtml_->setMasterCSS(css);
    }

    commentaryGroup_->end();
    commentaryGroup_->resizable(commentaryHtml_);

    // --- Dictionary tab ---
    dictionaryGroup_ = new Fl_Group(X + padding, tabY,
                                     W - 2 * padding, tabH, "Dictionary");
    dictionaryGroup_->begin();

    dictionaryChoice_ = new Fl_Choice(X + padding + 2, tabY + 2,
                                       W - 2 * padding - 4, choiceH);
    dictionaryChoice_->callback(onDictionaryModuleChange, this);

    dictionaryHtml_ = new HtmlWidget(X + padding + 2, tabY + choiceH + 4,
                                      W - 2 * padding - 4,
                                      tabH - choiceH - 6);

    // Load CSS
    std::ifstream cssStream2(cssFile);
    if (cssStream2.is_open()) {
        std::string css((std::istreambuf_iterator<char>(cssStream2)),
                         std::istreambuf_iterator<char>());
        dictionaryHtml_->setMasterCSS(css);
    }

    dictionaryGroup_->end();
    dictionaryGroup_->resizable(dictionaryHtml_);

    tabs_->end();
    tabs_->value(commentaryGroup_);

    end();
    resizable(tabs_);

    // Populate module choices
    populateCommentaryModules();
    populateDictionaryModules();
}

RightPane::~RightPane() = default;

void RightPane::showCommentary(const std::string& reference) {
    if (currentCommentary_.empty()) return;
    showCommentary(currentCommentary_, reference);
}

void RightPane::showCommentary(const std::string& moduleName,
                                const std::string& reference) {
    currentCommentary_ = moduleName;
    currentCommentaryRef_ = reference;

    std::string html = app_->swordManager().getCommentaryText(moduleName, reference);
    if (commentaryHtml_) {
        commentaryHtml_->setHtml(html);
    }

    // Switch to commentary tab
    if (tabs_) {
        tabs_->value(commentaryGroup_);
    }
}

void RightPane::showDictionaryEntry(const std::string& key) {
    if (currentDictionary_.empty()) {
        // Try to auto-select based on key format
        if (!key.empty()) {
            char prefix = key[0];
            if (prefix == 'H') {
                auto dicts = app_->swordManager().getDictionaryModules();
                for (const auto& d : dicts) {
                    if (d.name.find("Hebrew") != std::string::npos ||
                        d.name == "StrongsHebrew") {
                        setDictionaryModule(d.name);
                        break;
                    }
                }
            } else if (prefix == 'G') {
                auto dicts = app_->swordManager().getDictionaryModules();
                for (const auto& d : dicts) {
                    if (d.name.find("Greek") != std::string::npos ||
                        d.name == "StrongsGreek") {
                        setDictionaryModule(d.name);
                        break;
                    }
                }
            }
        }
    }

    if (!currentDictionary_.empty()) {
        showDictionaryEntry(currentDictionary_, key);
    }
}

void RightPane::showDictionaryEntry(const std::string& moduleName,
                                     const std::string& key) {
    currentDictionary_ = moduleName;
    currentDictKey_ = key;

    std::string html = app_->swordManager().getDictionaryEntry(moduleName, key);
    if (dictionaryHtml_) {
        dictionaryHtml_->setHtml(html);
    }

    // Switch to dictionary tab
    if (tabs_) {
        tabs_->value(dictionaryGroup_);
    }
}

void RightPane::setCommentaryModule(const std::string& moduleName) {
    currentCommentary_ = moduleName;

    // Update choice widget
    for (int i = 0; i < commentaryChoice_->size(); i++) {
        const Fl_Menu_Item& item = commentaryChoice_->menu()[i];
        if (item.label() && moduleName == item.label()) {
            commentaryChoice_->value(i);
            break;
        }
    }
}

void RightPane::setDictionaryModule(const std::string& moduleName) {
    currentDictionary_ = moduleName;

    // Update choice widget
    for (int i = 0; i < dictionaryChoice_->size(); i++) {
        const Fl_Menu_Item& item = dictionaryChoice_->menu()[i];
        if (item.label() && moduleName == item.label()) {
            dictionaryChoice_->value(i);
            break;
        }
    }
}

void RightPane::refresh() {
    if (!currentCommentary_.empty() && !currentCommentaryRef_.empty()) {
        showCommentary(currentCommentary_, currentCommentaryRef_);
    }
    if (!currentDictionary_.empty() && !currentDictKey_.empty()) {
        showDictionaryEntry(currentDictionary_, currentDictKey_);
    }
}

void RightPane::populateCommentaryModules() {
    if (!commentaryChoice_) return;
    commentaryChoice_->clear();

    auto mods = app_->swordManager().getCommentaryModules();
    for (const auto& mod : mods) {
        commentaryChoice_->add(mod.name.c_str());
    }

    if (commentaryChoice_->size() > 0) {
        commentaryChoice_->value(0);
        const Fl_Menu_Item& item = commentaryChoice_->menu()[0];
        if (item.label()) {
            currentCommentary_ = item.label();
        }
    }
}

void RightPane::populateDictionaryModules() {
    if (!dictionaryChoice_) return;
    dictionaryChoice_->clear();

    auto mods = app_->swordManager().getDictionaryModules();
    for (const auto& mod : mods) {
        dictionaryChoice_->add(mod.name.c_str());
    }

    if (dictionaryChoice_->size() > 0) {
        dictionaryChoice_->value(0);
        const Fl_Menu_Item& item = dictionaryChoice_->menu()[0];
        if (item.label()) {
            currentDictionary_ = item.label();
        }
    }
}

void RightPane::onCommentaryModuleChange(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<RightPane*>(data);
    const Fl_Menu_Item* item = self->commentaryChoice_->mvalue();
    if (item && item->label()) {
        self->currentCommentary_ = item->label();
        if (!self->currentCommentaryRef_.empty()) {
            self->showCommentary(self->currentCommentary_,
                                 self->currentCommentaryRef_);
        }
    }
}

void RightPane::onDictionaryModuleChange(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<RightPane*>(data);
    const Fl_Menu_Item* item = self->dictionaryChoice_->mvalue();
    if (item && item->label()) {
        self->currentDictionary_ = item->label();
        if (!self->currentDictKey_.empty()) {
            self->showDictionaryEntry(self->currentDictionary_,
                                      self->currentDictKey_);
        }
    }
}

} // namespace verdad
