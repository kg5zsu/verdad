#include "tags/TagManager.h"
#include <fstream>
#include <sstream>
#include <algorithm>

namespace verdad {

TagManager::TagManager() = default;
TagManager::~TagManager() {
    if (!filepath_.empty()) {
        save();
    }
}

bool TagManager::load(const std::string& filepath) {
    filepath_ = filepath;
    tags_.clear();
    verseTags_.clear();

    std::ifstream file(filepath);
    if (!file.is_open()) return true; // No file yet is OK

    std::string line;
    std::string section;

    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Section headers
        if (line == "[tags]") {
            section = "tags";
            continue;
        }
        if (line == "[verses]") {
            section = "verses";
            continue;
        }

        if (section == "tags") {
            // Format: name|color
            size_t sep = line.find('|');
            if (sep != std::string::npos) {
                Tag tag;
                tag.name = line.substr(0, sep);
                tag.color = line.substr(sep + 1);
                tags_[tag.name] = tag;
            }
        } else if (section == "verses") {
            // Format: verseKey|tag1,tag2,tag3
            size_t sep = line.find('|');
            if (sep != std::string::npos) {
                std::string verseKey = line.substr(0, sep);
                std::string tagList = line.substr(sep + 1);

                std::istringstream iss(tagList);
                std::string tagName;
                while (std::getline(iss, tagName, ',')) {
                    if (!tagName.empty()) {
                        verseTags_[verseKey].insert(tagName);
                    }
                }
            }
        }
    }

    return true;
}

bool TagManager::save(const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) return false;

    file << "# Verdad verse tags\n";

    // Write tags
    file << "[tags]\n";
    for (const auto& pair : tags_) {
        file << pair.second.name << "|" << pair.second.color << "\n";
    }

    // Write verse-tag associations
    file << "[verses]\n";
    for (const auto& pair : verseTags_) {
        if (pair.second.empty()) continue;
        file << pair.first << "|";
        bool first = true;
        for (const auto& tag : pair.second) {
            if (!first) file << ",";
            file << tag;
            first = false;
        }
        file << "\n";
    }

    filepath_ = filepath;
    return true;
}

bool TagManager::save() {
    if (filepath_.empty()) return false;
    return save(filepath_);
}

bool TagManager::createTag(const std::string& name, const std::string& color) {
    if (tags_.find(name) != tags_.end()) return false;

    Tag tag;
    tag.name = name;
    tag.color = color;
    tags_[name] = tag;
    return true;
}

bool TagManager::deleteTag(const std::string& name) {
    auto it = tags_.find(name);
    if (it == tags_.end()) return false;

    tags_.erase(it);

    // Remove from all verses
    for (auto& pair : verseTags_) {
        pair.second.erase(name);
    }

    return true;
}

bool TagManager::renameTag(const std::string& oldName, const std::string& newName) {
    auto it = tags_.find(oldName);
    if (it == tags_.end()) return false;
    if (tags_.find(newName) != tags_.end()) return false;

    Tag tag = it->second;
    tag.name = newName;
    tags_.erase(it);
    tags_[newName] = tag;

    // Update verse associations
    for (auto& pair : verseTags_) {
        if (pair.second.erase(oldName) > 0) {
            pair.second.insert(newName);
        }
    }

    return true;
}

void TagManager::setTagColor(const std::string& name, const std::string& color) {
    auto it = tags_.find(name);
    if (it != tags_.end()) {
        it->second.color = color;
    }
}

void TagManager::tagVerse(const std::string& verseKey, const std::string& tagName) {
    // Ensure tag exists
    if (tags_.find(tagName) == tags_.end()) {
        createTag(tagName);
    }
    verseTags_[verseKey].insert(tagName);
}

void TagManager::untagVerse(const std::string& verseKey, const std::string& tagName) {
    auto it = verseTags_.find(verseKey);
    if (it != verseTags_.end()) {
        it->second.erase(tagName);
        if (it->second.empty()) {
            verseTags_.erase(it);
        }
    }
}

std::vector<Tag> TagManager::getAllTags() const {
    std::vector<Tag> result;
    for (const auto& pair : tags_) {
        result.push_back(pair.second);
    }
    std::sort(result.begin(), result.end(),
              [](const Tag& a, const Tag& b) { return a.name < b.name; });
    return result;
}

std::vector<Tag> TagManager::getTagsForVerse(const std::string& verseKey) const {
    std::vector<Tag> result;
    auto it = verseTags_.find(verseKey);
    if (it != verseTags_.end()) {
        for (const auto& tagName : it->second) {
            auto tagIt = tags_.find(tagName);
            if (tagIt != tags_.end()) {
                result.push_back(tagIt->second);
            }
        }
    }
    std::sort(result.begin(), result.end(),
              [](const Tag& a, const Tag& b) { return a.name < b.name; });
    return result;
}

std::vector<std::string> TagManager::getVersesWithTag(const std::string& tagName) const {
    std::vector<std::string> result;
    for (const auto& pair : verseTags_) {
        if (pair.second.count(tagName) > 0) {
            result.push_back(pair.first);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool TagManager::verseHasTag(const std::string& verseKey,
                              const std::string& tagName) const {
    auto it = verseTags_.find(verseKey);
    if (it != verseTags_.end()) {
        return it->second.count(tagName) > 0;
    }
    return false;
}

int TagManager::getTagCount(const std::string& tagName) const {
    int count = 0;
    for (const auto& pair : verseTags_) {
        if (pair.second.count(tagName) > 0) {
            count++;
        }
    }
    return count;
}

} // namespace verdad
