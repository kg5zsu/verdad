#ifndef VERDAD_BOOKMARK_MANAGER_H
#define VERDAD_BOOKMARK_MANAGER_H

#include <string>
#include <vector>
#include <memory>

namespace verdad {

/// A bookmark entry
struct Bookmark {
    std::string title;      // Display title
    std::string verseKey;   // Verse reference
    std::string module;     // Module name
    std::string note;       // Optional user note
    std::string folder;     // Folder/category for organization
};

/// A bookmark folder
struct BookmarkFolder {
    std::string name;
    std::vector<Bookmark> bookmarks;
    std::vector<BookmarkFolder> subfolders;
};

/// Manages bookmarks - persisted to a file
class BookmarkManager {
public:
    BookmarkManager();
    ~BookmarkManager();

    /// Load bookmarks from file
    bool load(const std::string& filepath);

    /// Save bookmarks to file
    bool save(const std::string& filepath);

    /// Save to last loaded file
    bool save();

    /// Add a bookmark
    void addBookmark(const Bookmark& bm);

    /// Remove a bookmark by index in the flat list
    void removeBookmark(int index);

    /// Get all bookmarks (flat list)
    const std::vector<Bookmark>& getBookmarks() const;

    /// Get bookmarks in a specific folder
    std::vector<Bookmark> getBookmarksInFolder(const std::string& folder) const;

    /// Get all folder names
    std::vector<std::string> getFolders() const;

    /// Create a folder
    void createFolder(const std::string& name);

    /// Check if a verse is bookmarked
    bool isBookmarked(const std::string& verseKey, const std::string& module = "") const;

    /// Find bookmark by verse key
    const Bookmark* findBookmark(const std::string& verseKey,
                                 const std::string& module = "") const;

private:
    std::string filepath_;
    std::vector<Bookmark> bookmarks_;
    std::vector<std::string> folders_;
};

} // namespace verdad

#endif // VERDAD_BOOKMARK_MANAGER_H
