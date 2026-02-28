#ifndef VERDAD_APP_H
#define VERDAD_APP_H

#include <memory>
#include <string>
#include <FL/Enumerations.H>

namespace verdad {

class SwordManager;
class SearchIndexer;
class TagManager;
class MainWindow;

/// Main application class - owns all managers and the main window
class VerdadApp {
public:
    struct AppearanceSettings {
        std::string appFontName = "Helvetica";
        int appFontSize = 14;
        std::string textFontFamily = "DejaVu Serif";
        int textFontSize = 14;
        int hoverDelayMs = 1000;
    };

    VerdadApp();
    ~VerdadApp();

    /// Initialize the application. Returns true on success.
    bool initialize(int argc, char* argv[]);

    /// Run the application event loop. Returns exit code.
    int run();

    /// Get the SWORD manager
    SwordManager& swordManager() { return *swordMgr_; }
    const SwordManager& swordManager() const { return *swordMgr_; }

    /// Get the tag manager
    TagManager& tagManager() { return *tagMgr_; }

    /// Get the search indexer (may be nullptr if initialization failed)
    SearchIndexer* searchIndexer() { return searchIndexer_.get(); }
    const SearchIndexer* searchIndexer() const { return searchIndexer_.get(); }

    /// Get the main window
    MainWindow* mainWindow() { return mainWindow_.get(); }

    /// Get application data directory
    std::string getDataDir() const;

    /// Get application config directory
    std::string getConfigDir() const;

    /// Get the singleton instance
    static VerdadApp* instance() { return instance_; }

    /// Current appearance settings.
    const AppearanceSettings& appearanceSettings() const { return appearanceSettings_; }

    /// Update appearance settings and apply them immediately.
    void setAppearanceSettings(const AppearanceSettings& settings);

    /// Resolve configured UI font to FLTK font enum.
    Fl_Font appFont() const;

    /// Build runtime CSS overrides for HTML-rendered text panes.
    std::string textStyleOverrideCss() const;

private:
    static VerdadApp* instance_;

    std::unique_ptr<SwordManager> swordMgr_;
    std::unique_ptr<SearchIndexer> searchIndexer_;
    std::unique_ptr<TagManager> tagMgr_;
    std::unique_ptr<MainWindow> mainWindow_;
    AppearanceSettings appearanceSettings_;

    /// Ensure config directory exists
    void ensureConfigDir();

    /// Load user preferences
    void loadPreferences();

    /// Save user preferences
    void savePreferences();
};

} // namespace verdad

#endif // VERDAD_APP_H
