#ifndef VERDAD_UI_FONT_UTILS_H
#define VERDAD_UI_FONT_UTILS_H

#include "app/VerdadApp.h"
#include "ui/StyledTabs.h"

#include <FL/Fl_Browser_.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Input_.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Widget.H>

#include <algorithm>

namespace verdad {
namespace ui_font {

inline int clampUiFontSize(int size) {
    return std::clamp(size, 8, 36);
}

inline bool isBoldLabelFont(Fl_Font font) {
    switch (font) {
    case FL_HELVETICA_BOLD:
    case FL_HELVETICA_BOLD_ITALIC:
    case FL_COURIER_BOLD:
    case FL_COURIER_BOLD_ITALIC:
    case FL_TIMES_BOLD:
    case FL_TIMES_BOLD_ITALIC:
        return true;
    default:
        return false;
    }
}

inline void applyMenuFont(Fl_Menu_* menu, Fl_Font font, int size) {
    if (!menu) return;
    const int clampedSize = clampUiFontSize(size);
    menu->textfont(font);
    menu->textsize(clampedSize);
}

inline void applyTreeItemFont(Fl_Tree* tree, Fl_Font font, int size) {
    if (!tree) return;
    const int clampedSize = clampUiFontSize(size);
    tree->item_labelfont(font);
    tree->item_labelsize(clampedSize);
    for (Fl_Tree_Item* item = tree->first(); item; item = tree->next(item)) {
        item->labelfont(font);
        item->labelsize(clampedSize);
    }
}

inline void applyUiFontRecursively(Fl_Widget* w, Fl_Font font, Fl_Font boldFont, int size) {
    if (!w) return;
    const int clampedSize = clampUiFontSize(size);

    w->labelfont(isBoldLabelFont(w->labelfont()) ? boldFont : font);
    w->labelsize(clampedSize);

    if (auto* stabs = dynamic_cast<StyledTabs*>(w)) {
        stabs->setLabelFonts(font, boldFont);
    }

    if (auto* input = dynamic_cast<Fl_Input_*>(w)) {
        input->textfont(font);
        input->textsize(clampedSize);
    } else if (auto* choice = dynamic_cast<Fl_Choice*>(w)) {
        choice->textfont(font);
        choice->textsize(clampedSize);
    } else if (auto* browser = dynamic_cast<Fl_Browser_*>(w)) {
        browser->textfont(font);
        browser->textsize(clampedSize);
    } else if (auto* menu = dynamic_cast<Fl_Menu_*>(w)) {
        applyMenuFont(menu, font, clampedSize);
    }

    if (auto* tree = dynamic_cast<Fl_Tree*>(w)) {
        applyTreeItemFont(tree, font, clampedSize);
    }

    if (auto* group = dynamic_cast<Fl_Group*>(w)) {
        for (int i = 0; i < group->children(); ++i) {
            applyUiFontRecursively(group->child(i), font, boldFont, clampedSize);
        }
    }
}

inline bool currentAppUiFont(Fl_Font& font, Fl_Font& boldFont, int& size) {
    VerdadApp* app = VerdadApp::instance();
    if (!app) return false;

    font = app->appFont();
    size = clampUiFontSize(app->appearanceSettings().appFontSize);
    boldFont = app->boldFltkFont(font);
    return true;
}

inline void applyCurrentAppUiFont(Fl_Widget* w) {
    Fl_Font font = FL_HELVETICA;
    Fl_Font boldFont = FL_HELVETICA_BOLD;
    int size = FL_NORMAL_SIZE;
    if (currentAppUiFont(font, boldFont, size)) {
        applyUiFontRecursively(w, font, boldFont, size);
    }
}

inline void applyCurrentAppMenuFont(Fl_Menu_* menu) {
    Fl_Font font = FL_HELVETICA;
    Fl_Font boldFont = FL_HELVETICA_BOLD;
    int size = FL_NORMAL_SIZE;
    if (currentAppUiFont(font, boldFont, size)) {
        applyMenuFont(menu, font, size);
    }
}

} // namespace ui_font
} // namespace verdad

#endif // VERDAD_UI_FONT_UTILS_H
