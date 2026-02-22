#include "bookmarks/BookmarkManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace verdad {

BookmarkManager::BookmarkManager() = default;
BookmarkManager::~BookmarkManager() {
    if (!filepath_.empty()) {
        save();
    }
}

bool BookmarkManager::load(const std::string& filepath) {
    filepath_ = filepath;
    bookmarks_.clear();
    folders_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) return true; // No file yet is OK

    std::string line;
    std::string section;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line == "[folders]") {
            section = "folders";
            continue;
        }
        if (line == "[bookmarks]") {
            section = "bookmarks";
            continue;
        }

        if (section == "folders") {
            folders_.push_back(line);
        } else if (section == "bookmarks") {
            // Format: title|verseKey|module|folder|note
            Bookmark bm;
            std::istringstream iss(line);
            std::string token;

            if (std::getline(iss, token, '|')) bm.title = token;
            if (std::getline(iss, token, '|')) bm.verseKey = token;
            if (std::getline(iss, token, '|')) bm.module = token;
            if (std::getline(iss, token, '|')) bm.folder = token;
            if (std::getline(iss, token, '|')) bm.note = token;

            if (!bm.verseKey.empty()) {
                bookmarks_.push_back(bm);
            }
        }
    }

    return true;
}

bool BookmarkManager::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Verdad bookmarks\n";

    file << "[folders]\n";
    for (const auto& folder : folders_) {
        file << folder << "\n";
    }

    file << "[bookmarks]\n";
    for (const auto& bm : bookmarks_) {
        file << bm.title << "|" << bm.verseKey << "|"
             << bm.module << "|" << bm.folder << "|" << bm.note << "\n";
    }

    filepath_ = filepath;
    return true;
}

bool BookmarkManager::save() {
    if (filepath_.empty()) return false;
    return save(filepath_);
}

void BookmarkManager::addBookmark(const Bookmark& bm) {
    bookmarks_.push_back(bm);
}

void BookmarkManager::removeBookmark(int index) {
    if (index >= 0 && index < static_cast<int>(bookmarks_.size())) {
        bookmarks_.erase(bookmarks_.begin() + index);
    }
}

const std::vector<Bookmark>& BookmarkManager::getBookmarks() const {
    return bookmarks_;
}

std::vector<Bookmark> BookmarkManager::getBookmarksInFolder(
    const std::string& folder) const {
    std::vector<Bookmark> result;
    for (const auto& bm : bookmarks_) {
        if (bm.folder == folder) {
            result.push_back(bm);
        }
    }
    return result;
}

std::vector<std::string> BookmarkManager::getFolders() const {
    return folders_;
}

void BookmarkManager::createFolder(const std::string& name) {
    if (std::find(folders_.begin(), folders_.end(), name) == folders_.end()) {
        folders_.push_back(name);
    }
}

bool BookmarkManager::isBookmarked(const std::string& verseKey,
                                    const std::string& module) const {
    return findBookmark(verseKey, module) != nullptr;
}

const Bookmark* BookmarkManager::findBookmark(const std::string& verseKey,
                                               const std::string& module) const {
    for (const auto& bm : bookmarks_) {
        if (bm.verseKey == verseKey) {
            if (module.empty() || bm.module == module) {
                return &bm;
            }
        }
    }
    return nullptr;
}

} // namespace verdad
