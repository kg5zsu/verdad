#ifndef VERDAD_VERSE_CONTEXT_H
#define VERDAD_VERSE_CONTEXT_H

#include <FL/Fl_Menu_Button.H>
#include <string>
#include <vector>

namespace verdad {

class VerdadApp;

/// Right-click context menu for verse/word interactions.
/// Provides options like:
/// - Search for Strong's number
/// - Add/remove tag
/// - Copy verse text
class VerseContext {
public:
    struct SelectionContext {
        bool hasSelection = false;
        std::string text;
        std::string wholeWordText;
        bool startsInsideWord = false;
        bool endsInsideWord = false;
        int startVerse = 0;
        int endVerse = 0;
    };

    VerseContext(VerdadApp* app);
    ~VerseContext();

    /// Show context menu at position for a word/verse
    /// @param word      The word that was right-clicked
    /// @param href      The href attribute (may contain strongs: link)
    /// @param strong    Strong's token(s) from data-strong attribute
    /// @param morph     Morph token(s) from data-morph attribute
    /// @param module    Source module for this hit (parallel-aware)
    /// @param verseKey  Current verse reference
    /// @param screenX   Screen X position
    /// @param screenY   Screen Y position
    void show(const std::string& word, const std::string& href,
              const std::string& strong, const std::string& morph,
              const std::string& module,
              const std::string& book,
              int chapter,
              int verse,
              bool paragraphMode,
              const SelectionContext& selection,
              int screenX, int screenY);

private:
    enum class CopyActionKind {
        Verse,
        Verses,
        Selection
    };

    VerdadApp* app_;
    std::string currentWord_;
    std::string currentHref_;
    std::string currentStrong_;
    std::string currentMorph_;
    std::string currentContextModule_;
    std::string currentDictionaryLookupKey_;
    std::string currentBook_;
    SelectionContext currentSelection_;
    int currentChapter_ = 0;
    int currentVerse_ = 0;
    bool currentParagraphMode_ = false;

    struct StrongsMenuAction {
        VerseContext* owner = nullptr;
        std::string strongsNumber;
        bool dictionaryLookup = false;
    };
    std::vector<StrongsMenuAction> strongActions_;

    struct CopyMenuAction {
        VerseContext* owner = nullptr;
        CopyActionKind kind = CopyActionKind::Verse;
        bool referenceFirst = true;
        int startVerse = 0;
        int endVerse = 0;
    };
    std::vector<CopyMenuAction> copyActions_;

    /// Extract all Strong's numbers from href/data-strong payload.
    std::vector<std::string> extractStrongsNumbers(
        const std::string& href,
        const std::string& strong = "") const;

    std::string currentVerseKey() const;
    std::string verseKeyForNumber(int verse) const;
    std::string referenceLabel(int startVerse, int endVerse) const;
    std::string singleVerseText(int verse, bool includeVerseNumber) const;
    std::string multiVerseText(int startVerse, int endVerse) const;
    std::string selectionCopyText() const;
    std::string formattedCopyText(const std::string& body,
                                  int startVerse,
                                  int endVerse,
                                  bool referenceFirst) const;
    void copyToClipboard(const std::string& text) const;
    void addCopyMenuItem(Fl_Menu_Button& menu,
                         const char* label,
                         CopyActionKind kind,
                         bool referenceFirst,
                         int startVerse,
                         int endVerse);

    // Menu action callbacks
    static void onStrongsAction(Fl_Widget* w, void* data);
    static void onAddTag(Fl_Widget* w, void* data);
    static void onCopyAction(Fl_Widget* w, void* data);
    static void onSearchWord(Fl_Widget* w, void* data);
    static void onLookupDictionary(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_VERSE_CONTEXT_H
