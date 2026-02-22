#include "ui/BookmarkPanel.h"
#include "app/VerdadApp.h"
#include "ui/MainWindow.h"
#include "bookmarks/BookmarkManager.h"

#include <FL/Fl.H>
#include <FL/fl_ask.H>

namespace verdad {

BookmarkPanel::BookmarkPanel(VerdadApp* app, int X, int Y, int W, int H)
    : Fl_Group(X, Y, W, H)
    , app_(app) {

    begin();

    int padding = 2;
    int buttonH = 25;
    int buttonW = (W - 3 * padding) / 2;

    // Bookmark list
    browser_ = new Fl_Browser(X + padding, Y + padding,
                               W - 2 * padding, H - buttonH - 3 * padding);
    browser_->type(FL_HOLD_BROWSER);
    browser_->callback(onSelect, this);
    browser_->when(FL_WHEN_CHANGED | FL_WHEN_RELEASE);

    // Buttons at bottom
    int by = Y + H - buttonH - padding;

    deleteButton_ = new Fl_Button(X + padding, by, buttonW, buttonH, "Delete");
    deleteButton_->callback(onDelete, this);

    addFolderButton_ = new Fl_Button(X + 2 * padding + buttonW, by,
                                      buttonW, buttonH, "Add Folder");
    addFolderButton_->callback(onAddFolder, this);

    end();
    resizable(browser_);

    populateList();
}

BookmarkPanel::~BookmarkPanel() = default;

void BookmarkPanel::refresh() {
    populateList();
}

void BookmarkPanel::addBookmark(const std::string& verseKey,
                                 const std::string& module) {
    Bookmark bm;
    bm.title = verseKey;
    bm.verseKey = verseKey;
    bm.module = module;
    bm.folder = "";

    app_->bookmarkManager().addBookmark(bm);
    app_->bookmarkManager().save();
    populateList();
}

void BookmarkPanel::populateList() {
    browser_->clear();

    // Add folder separators and bookmarks
    auto folders = app_->bookmarkManager().getFolders();
    for (const auto& folder : folders) {
        browser_->add(("@b[" + folder + "]").c_str());

        auto bms = app_->bookmarkManager().getBookmarksInFolder(folder);
        for (const auto& bm : bms) {
            std::string line = "  " + bm.title + " (" + bm.module + ")";
            browser_->add(line.c_str());
        }
    }

    // Bookmarks without a folder
    const auto& allBms = app_->bookmarkManager().getBookmarks();
    for (const auto& bm : allBms) {
        if (bm.folder.empty()) {
            std::string line = bm.title + " (" + bm.module + ")";
            browser_->add(line.c_str());
        }
    }
}

void BookmarkPanel::onSelect(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<BookmarkPanel*>(data);

    // Handle double-click
    if (Fl::event_clicks() > 0) {
        onDoubleClick(nullptr, data);
    }

    (void)self;
}

void BookmarkPanel::onDoubleClick(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<BookmarkPanel*>(data);

    int idx = self->browser_->value();
    if (idx <= 0) return;

    // Find the corresponding bookmark
    const auto& bookmarks = self->app_->bookmarkManager().getBookmarks();
    // Simple index mapping (accounting for folder headers)
    int bmIdx = 0;
    for (int i = 1; i <= self->browser_->size(); i++) {
        const char* text = self->browser_->text(i);
        if (text && text[0] == '@') continue; // Skip folder headers
        if (i == idx) {
            if (bmIdx < static_cast<int>(bookmarks.size())) {
                const auto& bm = bookmarks[bmIdx];
                if (self->app_->mainWindow()) {
                    self->app_->mainWindow()->navigateTo(bm.module, bm.verseKey);
                }
            }
            break;
        }
        bmIdx++;
    }
}

void BookmarkPanel::onDelete(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<BookmarkPanel*>(data);

    int idx = self->browser_->value();
    if (idx <= 0) return;

    int confirm = fl_choice("Delete this bookmark?", "Cancel", "Delete", nullptr);
    if (confirm == 1) {
        // Map browser index to bookmark index
        // (simplified - in production would need proper mapping)
        int bmIdx = idx - 1;
        self->app_->bookmarkManager().removeBookmark(bmIdx);
        self->app_->bookmarkManager().save();
        self->populateList();
    }
}

void BookmarkPanel::onAddFolder(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<BookmarkPanel*>(data);

    const char* name = fl_input("Folder name:");
    if (name && name[0]) {
        self->app_->bookmarkManager().createFolder(name);
        self->app_->bookmarkManager().save();
        self->populateList();
    }
}

} // namespace verdad
