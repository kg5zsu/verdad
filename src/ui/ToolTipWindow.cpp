#include "ui/ToolTipWindow.h"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include <algorithm>

namespace verdad {

ToolTipWindow::ToolTipWindow()
    : Fl_Menu_Window(1, 1) {
    set_override(); // Don't take focus
    clear_border();
    box(FL_BORDER_BOX);
    color(fl_rgb_color(255, 255, 225)); // Light yellow tooltip background

    label_ = new Fl_Box(2, 2, 1, 1);
    label_->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    label_->box(FL_NO_BOX);
    label_->labelfont(FL_HELVETICA);
    label_->labelsize(12);
    label_->labelcolor(FL_BLACK);

    end();
    hide();
}

ToolTipWindow::~ToolTipWindow() = default;

void ToolTipWindow::showAt(int screenX, int screenY, const std::string& html) {
    // Strip HTML tags for plain text display in the tooltip
    // (We use a simple Fl_Box rather than full HTML rendering for tooltips)
    std::string plainText;
    bool inTag = false;
    bool lastWasSpace = false;

    for (size_t i = 0; i < html.size(); i++) {
        char c = html[i];
        if (c == '<') {
            // Check for <br/> or <br>
            if (html.substr(i, 4) == "<br>" || html.substr(i, 5) == "<br/>") {
                plainText += '\n';
                lastWasSpace = false;
            }
            inTag = true;
        } else if (c == '>') {
            inTag = false;
        } else if (!inTag) {
            if (c == '&') {
                // Handle basic HTML entities
                size_t semi = html.find(';', i);
                if (semi != std::string::npos && semi - i < 8) {
                    std::string entity = html.substr(i, semi - i + 1);
                    if (entity == "&amp;") plainText += '&';
                    else if (entity == "&lt;") plainText += '<';
                    else if (entity == "&gt;") plainText += '>';
                    else if (entity == "&nbsp;") plainText += ' ';
                    else if (entity == "&quot;") plainText += '"';
                    else plainText += entity;
                    i = semi;
                } else {
                    plainText += c;
                }
            } else if (c == '\n' || c == '\r') {
                if (!lastWasSpace) {
                    plainText += ' ';
                    lastWasSpace = true;
                }
            } else {
                plainText += c;
                lastWasSpace = (c == ' ');
            }
        }
    }

    // Trim whitespace
    while (!plainText.empty() && plainText.back() == ' ') plainText.pop_back();
    while (!plainText.empty() && plainText.front() == ' ')
        plainText.erase(plainText.begin());

    if (plainText.empty()) {
        hideTooltip();
        return;
    }

    currentText_ = plainText;

    // Measure text to size the window
    fl_font(FL_HELVETICA, 12);

    int maxW = 350;
    int textW = 0, textH = 0;

    // Split into lines and measure
    fl_measure(plainText.c_str(), textW, textH, 0);

    if (textW > maxW) textW = maxW;
    textW = std::max(textW, 100);
    textH = std::max(textH, 20);

    // Re-measure with wrapping
    int wrapW = textW;
    fl_measure(plainText.c_str(), wrapW, textH, 1);
    textH += 8; // Padding

    int winW = textW + 8;
    int winH = textH + 8;

    // Position tooltip, keeping on screen
    int sx = screenX;
    int sy = screenY;
    if (sx + winW > Fl::w()) sx = Fl::w() - winW;
    if (sy + winH > Fl::h()) sy = screenY - winH - 5;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;

    resize(sx, sy, winW, winH);
    label_->resize(4, 4, winW - 8, winH - 8);
    label_->copy_label(plainText.c_str());

    shown_ = true;
    show();
}

void ToolTipWindow::hideTooltip() {
    shown_ = false;
    hide();
}

void ToolTipWindow::draw() {
    // Draw background
    fl_color(color());
    fl_rectf(0, 0, w(), h());

    // Draw border
    fl_color(FL_DARK3);
    fl_rect(0, 0, w(), h());

    // Draw label
    draw_children();
}

int ToolTipWindow::handle(int event) {
    switch (event) {
    case FL_PUSH:
    case FL_RELEASE:
        hideTooltip();
        return 1;
    default:
        return Fl_Menu_Window::handle(event);
    }
}

} // namespace verdad
