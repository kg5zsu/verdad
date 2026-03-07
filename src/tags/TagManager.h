#ifndef VERDAD_TAG_MANAGER_H
#define VERDAD_TAG_MANAGER_H

#include <map>
#include <set>
#include <string>
#include <vector>

struct sqlite3;

namespace verdad {

/// A tag that can be applied to verses
struct Tag {
    std::string name;
    std::string color;   // hex color like "#4a86c8"
};

/// Manages verse tags persisted in an SQLite database
class TagManager {
public:
    TagManager();
    ~TagManager();

    /// Load tags from database. Returns true on success.
    bool load(const std::string& filepath);

    /// Save tags to database. Returns true on success.
    bool save(const std::string& filepath);

    /// Save to the last loaded database
    bool save();

    /// Create a new tag. Returns true if created (false if already exists).
    bool createTag(const std::string& name, const std::string& color = "#4a86c8");

    /// Delete a tag and remove it from all verses
    bool deleteTag(const std::string& name);

    /// Rename a tag
    bool renameTag(const std::string& oldName, const std::string& newName);

    /// Set tag color
    void setTagColor(const std::string& name, const std::string& color);

    /// Add a tag to a verse
    /// @param verseKey  Canonical verse reference (e.g. "Genesis 1:1")
    /// @param tagName   Tag name
    void tagVerse(const std::string& verseKey, const std::string& tagName);

    /// Remove a tag from a verse
    void untagVerse(const std::string& verseKey, const std::string& tagName);

    /// Get all tags
    std::vector<Tag> getAllTags() const;

    /// Get tags for a specific verse
    std::vector<Tag> getTagsForVerse(const std::string& verseKey) const;

    /// Get all verses with a specific tag
    std::vector<std::string> getVersesWithTag(const std::string& tagName) const;

    /// Check if a verse has a specific tag
    bool verseHasTag(const std::string& verseKey, const std::string& tagName) const;

    /// Get the number of verses with a given tag
    int getTagCount(const std::string& tagName) const;

private:
    bool openDatabase(const std::string& filepath);
    void closeDatabase();
    bool loadFromDatabase();
    bool persistToDatabase();
    bool hasStoredData() const;
    bool importLegacyFile(const std::string& legacyPath);

    std::string filepath_;
    sqlite3* db_ = nullptr;
    std::map<std::string, Tag> tags_;                        // tagName -> Tag
    std::map<std::string, std::set<std::string>> verseTags_; // verseKey -> set of tag names
    bool dirty_ = false;
};

} // namespace verdad

#endif // VERDAD_TAG_MANAGER_H
