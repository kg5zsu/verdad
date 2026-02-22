#ifndef VERDAD_BOOKMARK_PANEL_H
#define VERDAD_BOOKMARK_PANEL_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <string>

namespace verdad {

class VerdadApp;

/// Panel showing bookmarks in the left pane
class BookmarkPanel : public Fl_Group {
public:
    BookmarkPanel(VerdadApp* app, int X, int Y, int W, int H);
    ~BookmarkPanel() override;

    /// Refresh the bookmark list
    void refresh();

    /// Add a bookmark for the given verse
    void addBookmark(const std::string& verseKey, const std::string& module);

private:
    VerdadApp* app_;
    Fl_Browser* browser_;
    Fl_Button* deleteButton_;
    Fl_Button* addFolderButton_;

    /// Populate the browser with bookmarks
    void populateList();

    // Callbacks
    static void onSelect(Fl_Widget* w, void* data);
    static void onDoubleClick(Fl_Widget* w, void* data);
    static void onDelete(Fl_Widget* w, void* data);
    static void onAddFolder(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_BOOKMARK_PANEL_H
