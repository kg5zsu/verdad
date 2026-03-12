#ifndef VERDAD_VERSE_LIST_COPY_MENU_H
#define VERDAD_VERSE_LIST_COPY_MENU_H

#include "ui/UiFontUtils.h"
#include "sword/SwordManager.h"

#include <FL/Fl.H>
#include <FL/Fl_Menu_Button.H>

#include <cctype>
#include <string>
#include <vector>

namespace verdad {
namespace verse_list_copy {

struct Entry {
    std::string module;
    std::string key;
};

enum class Format {
    ReferenceOnly,
    ReferenceFirst,
    ReferenceLast
};

inline std::string trimCopy(const std::string& text) {
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

inline std::string referenceLabel(const std::string& reference) {
    return std::string("(") + reference + ")";
}

inline std::string formatEntry(const std::string& reference,
                               const std::string& body,
                               Format format) {
    std::string ref = trimCopy(reference);
    if (ref.empty()) return "";

    if (format == Format::ReferenceOnly) {
        return ref;
    }

    std::string trimmedBody = trimCopy(body);
    if (trimmedBody.empty()) {
        return ref;
    }

    std::string label = referenceLabel(ref);
    const bool multiLine = trimmedBody.find('\n') != std::string::npos;
    if (format == Format::ReferenceFirst) {
        return multiLine ? label + "\n" + trimmedBody
                         : label + " " + trimmedBody;
    }
    return multiLine ? trimmedBody + "\n" + label
                     : trimmedBody + " " + label;
}

inline std::string buildCopyText(SwordManager& swordManager,
                                 const std::vector<Entry>& entries,
                                 Format format) {
    std::string out;
    for (const auto& entry : entries) {
        std::string reference = trimCopy(entry.key);
        if (reference.empty()) continue;

        std::string line;
        if (format == Format::ReferenceOnly) {
            line = formatEntry(reference, "", format);
        } else {
            line = formatEntry(reference,
                               swordManager.getVersePlainText(entry.module, reference),
                               format);
        }
        if (line.empty()) continue;

        if (!out.empty()) {
            out.push_back('\n');
        }
        out += line;
    }
    return out;
}

inline void copyToClipboard(const std::string& text) {
    if (text.empty()) return;
    Fl::copy(text.c_str(), static_cast<int>(text.size()), 0);
    Fl::copy(text.c_str(), static_cast<int>(text.size()), 1);
}

inline void showVerseListCopyMenu(SwordManager& swordManager,
                                  const std::vector<Entry>& entries,
                                  int screenX,
                                  int screenY) {
    if (entries.empty()) return;

    constexpr const char* kRefOnlyLabel = "Copy List: Refs only";
    constexpr const char* kRefFirstLabel = "Copy Verses: Refs first";
    constexpr const char* kRefLastLabel = "Copy Verses: Refs last";

    Fl_Menu_Button menu(screenX, screenY, 0, 0);
    ui_font::applyCurrentAppMenuFont(&menu);
    menu.add(kRefOnlyLabel);
    menu.add(kRefFirstLabel);
    menu.add(kRefLastLabel);

    const Fl_Menu_Item* picked = menu.popup();
    if (!picked || !picked->label()) return;

    Format format = Format::ReferenceOnly;
    std::string label = picked->label();
    if (label == kRefFirstLabel) {
        format = Format::ReferenceFirst;
    } else if (label == kRefLastLabel) {
        format = Format::ReferenceLast;
    }

    copyToClipboard(buildCopyText(swordManager, entries, format));
}

} // namespace verse_list_copy
} // namespace verdad

#endif // VERDAD_VERSE_LIST_COPY_MENU_H
