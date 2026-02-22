#ifndef VERDAD_MODULE_PANEL_H
#define VERDAD_MODULE_PANEL_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Tree.H>
#include <string>

namespace verdad {

class VerdadApp;

/// Panel showing available SWORD modules organized by type
class ModulePanel : public Fl_Group {
public:
    ModulePanel(VerdadApp* app, int X, int Y, int W, int H);
    ~ModulePanel() override;

    /// Refresh the module tree
    void refresh();

private:
    VerdadApp* app_;
    Fl_Tree* tree_;

    /// Populate the tree with available modules
    void populateTree();

    // Callbacks
    static void onTreeSelect(Fl_Widget* w, void* data);
    static void onTreeDoubleClick(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_MODULE_PANEL_H
