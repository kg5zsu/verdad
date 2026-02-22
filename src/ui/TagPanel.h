#ifndef VERDAD_TAG_PANEL_H
#define VERDAD_TAG_PANEL_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Button.H>
#include <string>

namespace verdad {

class VerdadApp;

/// Panel showing tags and tagged verses in the left pane
class TagPanel : public Fl_Group {
public:
    TagPanel(VerdadApp* app, int X, int Y, int W, int H);
    ~TagPanel() override;

    /// Refresh the tag list
    void refresh();

    /// Show dialog to add a tag to a verse
    void showAddTagDialog(const std::string& verseKey);

private:
    VerdadApp* app_;

    // Tag list at top
    Fl_Browser* tagBrowser_;

    // Verses for selected tag at bottom
    Fl_Browser* verseBrowser_;

    // Buttons
    Fl_Button* newTagButton_;
    Fl_Button* deleteTagButton_;
    Fl_Button* renameTagButton_;

    /// Populate tag list
    void populateTags();

    /// Populate verse list for selected tag
    void populateVerses(const std::string& tagName);

    // Callbacks
    static void onTagSelect(Fl_Widget* w, void* data);
    static void onVerseSelect(Fl_Widget* w, void* data);
    static void onVerseDoubleClick(Fl_Widget* w, void* data);
    static void onNewTag(Fl_Widget* w, void* data);
    static void onDeleteTag(Fl_Widget* w, void* data);
    static void onRenameTag(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_TAG_PANEL_H
