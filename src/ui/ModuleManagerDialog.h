#ifndef VERDAD_MODULE_MANAGER_DIALOG_H
#define VERDAD_MODULE_MANAGER_DIALOG_H

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <memory>
#include <string>
#include <vector>

namespace sword {
class InstallMgr;
class InstallSource;
}

class Fl_Choice;
class Fl_Hold_Browser;
class Fl_Box;
class Fl_Button;

namespace verdad {

class VerdadApp;

/// Basic SWORD module manager UI (sources + install/update).
class ModuleManagerDialog : public Fl_Double_Window {
public:
    ModuleManagerDialog(VerdadApp* app, int W = 980, int H = 620);
    ~ModuleManagerDialog() override;

    /// Show dialog modally.
    void openModal();

private:
    struct SourceRow {
        std::string caption;
        std::string type;      // FTP/HTTP/HTTPS/SFTP/DIR
        std::string source;    // host or local dir
        std::string directory; // remote path
        bool isLocal = false;
        sword::InstallSource* remoteSource = nullptr; // owned by InstallMgr
    };

    struct ModuleRow {
        std::string sourceCaption;
        std::string sourceType;
        std::string sourcePath;    // local dir for DIR sources
        std::string moduleName;
        std::string moduleType;
        std::string language;
        std::string installedVersion;
        std::string availableVersion;
        std::string statusText;
        bool installed = false;
        bool updateAvailable = false;
        bool isBible = false;
        sword::InstallSource* remoteSource = nullptr; // null for local
    };

    VerdadApp* app_ = nullptr;
    std::string installMgrPath_;
    std::unique_ptr<sword::InstallMgr> installMgr_;

    Fl_Box* warningBox_ = nullptr;
    Fl_Choice* sourceChoice_ = nullptr;
    Fl_Hold_Browser* moduleBrowser_ = nullptr;
    Fl_Box* statusBox_ = nullptr;
    Fl_Button* refreshSourceButton_ = nullptr;
    Fl_Button* installButton_ = nullptr;

    std::vector<SourceRow> sources_;
    std::vector<ModuleRow> modules_;
    std::vector<int> visibleModuleRows_;

    void buildUi();
    void initializeInstallMgr();
    void refreshSources(bool refreshRemoteContent);
    void refreshModules();
    void reloadModuleRows();
    void repopulateSourceChoice();
    void repopulateModuleBrowser();
    int selectedVisibleRow() const;

    void addRemoteSource();
    void addLocalSource();
    void removeSelectedSource();
    void refreshSelectedSource();
    void installOrUpdateSelectedModule();
    bool confirmRemoteNetworkUse();

    static void onSourceChanged(Fl_Widget* w, void* data);
    static void onRefreshSource(Fl_Widget* w, void* data);
    static void onRefreshAll(Fl_Widget* w, void* data);
    static void onAddRemote(Fl_Widget* w, void* data);
    static void onAddLocal(Fl_Widget* w, void* data);
    static void onRemoveSource(Fl_Widget* w, void* data);
    static void onInstallUpdate(Fl_Widget* w, void* data);
};

} // namespace verdad

#endif // VERDAD_MODULE_MANAGER_DIALOG_H
