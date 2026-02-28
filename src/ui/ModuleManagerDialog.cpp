#include "ui/ModuleManagerDialog.h"

#include "app/VerdadApp.h"
#include "search/SearchIndexer.h"
#include "sword/SwordManager.h"
#include "ui/MainWindow.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>

#include <installmgr.h>
#include <markupfiltmgr.h>
#include <swconfig.h>
#include <swmgr.h>
#include <swmodule.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <map>
#include <sys/stat.h>

namespace verdad {
namespace {

std::string trimCopy(const std::string& s) {
    size_t start = 0;
    while (start < s.size() &&
           std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start &&
           std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::string upperCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

std::string safeText(const char* s) {
    return s ? std::string(s) : std::string();
}

bool startsWithNoCase(const std::string& text, const std::string& prefix) {
    if (prefix.size() > text.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        char a = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
        char b = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) return false;
    }
    return true;
}

bool ensureDir(const std::string& path) {
    if (path.empty()) return false;
    struct stat st;
    if (stat(path.c_str(), &st) == 0) return S_ISDIR(st.st_mode);
    return mkdir(path.c_str(), 0755) == 0;
}

std::string userHomeDir() {
    const char* home = std::getenv("HOME");
    return home ? std::string(home) : std::string();
}

std::string versionOfModule(sword::SWModule* mod) {
    if (!mod) return "";
    const char* v = mod->getConfigEntry("Version");
    return v ? v : "";
}

bool isBibleType(const std::string& type) {
    return startsWithNoCase(type, "Biblical Text");
}

} // namespace

ModuleManagerDialog::ModuleManagerDialog(VerdadApp* app, int W, int H)
    : Fl_Double_Window(W, H, "Module Manager")
    , app_(app) {
    std::string home = userHomeDir();
    installMgrPath_ = home.empty() ? std::string("./InstallMgr")
                                   : home + "/.sword/InstallMgr";

    buildUi();
    initializeInstallMgr();
    refreshSources(false);
    refreshModules();
}

ModuleManagerDialog::~ModuleManagerDialog() = default;

void ModuleManagerDialog::openModal() {
    set_modal();
    show();
    while (shown()) {
        Fl::wait();
    }
}

void ModuleManagerDialog::buildUi() {
    begin();

    warningBox_ = new Fl_Box(10, 10, w() - 20, 70,
        "Warning: Module download sites can reveal user activity over the network.\n"
        "If you live in a persecuted country and do not want to risk detection,\n"
        "do not use remote sources. Use local module sources only.");
    warningBox_->box(FL_BORDER_BOX);
    warningBox_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);

    int y = 88;
    sourceChoice_ = new Fl_Choice(90, y, 250, 26, "Source:");
    sourceChoice_->callback(onSourceChanged, this);

    Fl_Button* addRemoteBtn = new Fl_Button(350, y, 110, 26, "Add Remote...");
    addRemoteBtn->callback(onAddRemote, this);

    Fl_Button* addLocalBtn = new Fl_Button(465, y, 100, 26, "Add Local...");
    addLocalBtn->callback(onAddLocal, this);

    Fl_Button* removeBtn = new Fl_Button(570, y, 95, 26, "Remove");
    removeBtn->callback(onRemoveSource, this);

    refreshSourceButton_ = new Fl_Button(670, y, 95, 26, "Refresh");
    refreshSourceButton_->callback(onRefreshSource, this);

    Fl_Button* refreshAllBtn = new Fl_Button(770, y, 110, 26, "Refresh All");
    refreshAllBtn->callback(onRefreshAll, this);

    int browserY = y + 34;
    moduleBrowser_ = new Fl_Hold_Browser(10, browserY, w() - 20, h() - browserY - 74);
    static int colWidths[] = {180, 120, 170, 140, 90, 90, 0};
    moduleBrowser_->column_widths(colWidths);
    moduleBrowser_->add("Status\tModule\tType\tSource\tInstalled\tAvailable");

    statusBox_ = new Fl_Box(10, h() - 38, w() - 230, 26, "");
    statusBox_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    installButton_ = new Fl_Button(w() - 210, h() - 42, 110, 30, "Install/Update");
    installButton_->callback(onInstallUpdate, this);

    Fl_Button* closeBtn = new Fl_Button(w() - 90, h() - 42, 80, 30, "Close");
    closeBtn->callback(
        [](Fl_Widget* w, void*) {
            if (w && w->window()) w->window()->hide();
        }, nullptr);

    end();
}

void ModuleManagerDialog::initializeInstallMgr() {
    std::string home = userHomeDir();
    if (!home.empty()) {
        ensureDir(home + "/.sword");
    }
    ensureDir(installMgrPath_);

    installMgr_ = std::make_unique<sword::InstallMgr>(installMgrPath_.c_str());
    installMgr_->setFTPPassive(true);
    // We present our own explicit warning in the UI before remote operations.
    installMgr_->setUserDisclaimerConfirmed(true);
    installMgr_->readInstallConf();
}

void ModuleManagerDialog::refreshSources(bool refreshRemoteContent) {
    if (!installMgr_) initializeInstallMgr();
    if (!installMgr_) return;

    sources_.clear();
    installMgr_->readInstallConf();

    for (auto it = installMgr_->sources.begin(); it != installMgr_->sources.end(); ++it) {
        sword::InstallSource* src = it->second;
        if (!src) continue;
        SourceRow row;
        row.caption = safeText(src->caption);
        row.type = safeText(src->type);
        row.source = safeText(src->source);
        row.directory = safeText(src->directory);
        row.isLocal = false;
        row.remoteSource = src;
        sources_.push_back(std::move(row));
    }

    // Include local DIR sources from InstallMgr.conf.
    std::string confFile = installMgrPath_ + "/InstallMgr.conf";
    sword::SWConfig conf(confFile.c_str());
    auto secIt = conf.Sections.find("Sources");
    if (secIt != conf.Sections.end()) {
        auto begin = secIt->second.lower_bound("DIRSource");
        auto endIt = secIt->second.upper_bound("DIRSource");
        for (auto it = begin; it != endIt; ++it) {
            sword::InstallSource src("DIR", it->second.c_str());
            SourceRow row;
            row.caption = safeText(src.caption);
            row.type = "DIR";
            row.source = safeText(src.source);
            row.directory = safeText(src.directory);
            if (row.directory.empty()) row.directory = row.source;
            if (row.source.empty()) row.source = row.directory;
            row.isLocal = true;
            row.remoteSource = nullptr;
            sources_.push_back(std::move(row));
        }
    }

    std::sort(sources_.begin(), sources_.end(),
              [](const SourceRow& a, const SourceRow& b) {
                  return a.caption < b.caption;
              });

    repopulateSourceChoice();

    if (refreshRemoteContent) {
        for (const auto& src : sources_) {
            if (src.isLocal || !src.remoteSource) continue;
            statusBox_->copy_label(("Refreshing " + src.caption + "...").c_str());
            Fl::check();
            installMgr_->refreshRemoteSource(src.remoteSource);
        }
    }
}

void ModuleManagerDialog::refreshModules() {
    modules_.clear();

    sword::SWMgr localMgr(new sword::MarkupFilterMgr(sword::FMT_XHTML));
    std::map<std::string, sword::SWModule*> installed;
    for (auto it = localMgr.Modules.begin(); it != localMgr.Modules.end(); ++it) {
        if (!it->second) continue;
        installed[safeText(it->first)] = it->second;
    }

    for (const auto& src : sources_) {
        if (src.isLocal) {
            std::string localPath = !src.directory.empty() ? src.directory : src.source;
            if (localPath.empty()) continue;

            sword::SWMgr sourceMgr(localPath.c_str());
            for (auto it = sourceMgr.Modules.begin(); it != sourceMgr.Modules.end(); ++it) {
                sword::SWModule* mod = it->second;
                if (!mod) continue;

                std::string name = safeText(it->first);
                auto localIt = installed.find(name);
                std::string localVer = localIt != installed.end()
                    ? versionOfModule(localIt->second)
                    : "";
                std::string remoteVer = versionOfModule(mod);

                ModuleRow row;
                row.sourceCaption = src.caption;
                row.sourceType = src.type;
                row.sourcePath = localPath;
                row.moduleName = name;
                row.moduleType = safeText(mod->getType());
                row.language = safeText(mod->getLanguage());
                row.installedVersion = localVer;
                row.availableVersion = remoteVer;
                row.installed = (localIt != installed.end());
                row.updateAvailable = row.installed &&
                                      !remoteVer.empty() &&
                                      !localVer.empty() &&
                                      remoteVer != localVer;
                row.statusText = !row.installed
                                     ? "Not installed"
                                     : (row.updateAvailable
                                           ? "Update available"
                                           : "Installed");
                row.isBible = isBibleType(row.moduleType);
                row.remoteSource = nullptr;
                modules_.push_back(std::move(row));
            }
            continue;
        }

        if (!src.remoteSource) continue;
        sword::SWMgr* sourceMgr = src.remoteSource->getMgr();
        if (!sourceMgr) continue;

        auto statusMap = sword::InstallMgr::getModuleStatus(localMgr, *sourceMgr, false);
        for (auto it = sourceMgr->Modules.begin(); it != sourceMgr->Modules.end(); ++it) {
            sword::SWModule* mod = it->second;
            if (!mod) continue;

            std::string name = safeText(it->first);
            auto localIt = installed.find(name);
            std::string localVer = localIt != installed.end()
                ? versionOfModule(localIt->second)
                : "";
            std::string remoteVer = versionOfModule(mod);

            int flags = 0;
            auto st = statusMap.find(mod);
            if (st != statusMap.end()) {
                flags = st->second;
            }

            bool isNew = (flags & sword::InstallMgr::MODSTAT_NEW) != 0;
            bool isUpdated = (flags & sword::InstallMgr::MODSTAT_UPDATED) != 0;
            bool isSame = (flags & sword::InstallMgr::MODSTAT_SAMEVERSION) != 0;
            bool isOlder = (flags & sword::InstallMgr::MODSTAT_OLDER) != 0;

            ModuleRow row;
            row.sourceCaption = src.caption;
            row.sourceType = src.type;
            row.moduleName = name;
            row.moduleType = safeText(mod->getType());
            row.language = safeText(mod->getLanguage());
            row.installedVersion = localVer;
            row.availableVersion = remoteVer;
            row.remoteSource = src.remoteSource;
            row.isBible = isBibleType(row.moduleType);

            if (isNew) {
                row.installed = false;
                row.updateAvailable = false;
                row.statusText = "Not installed";
            } else if (isUpdated) {
                row.installed = true;
                row.updateAvailable = true;
                row.statusText = "Update available";
            } else if (isSame) {
                row.installed = true;
                row.updateAvailable = false;
                row.statusText = "Installed (up to date)";
            } else if (isOlder) {
                row.installed = true;
                row.updateAvailable = false;
                row.statusText = "Installed (local newer)";
            } else {
                row.installed = (localIt != installed.end());
                row.updateAvailable = row.installed &&
                                      !remoteVer.empty() &&
                                      !localVer.empty() &&
                                      remoteVer != localVer;
                row.statusText = !row.installed
                    ? "Not installed"
                    : (row.updateAvailable ? "Update available" : "Installed");
            }

            modules_.push_back(std::move(row));
        }
    }

    std::sort(modules_.begin(), modules_.end(),
              [](const ModuleRow& a, const ModuleRow& b) {
                  if (a.moduleName != b.moduleName) return a.moduleName < b.moduleName;
                  return a.sourceCaption < b.sourceCaption;
              });

    repopulateModuleBrowser();
}

void ModuleManagerDialog::repopulateSourceChoice() {
    if (!sourceChoice_) return;
    sourceChoice_->clear();
    sourceChoice_->add("All Sources");
    for (const auto& src : sources_) {
        sourceChoice_->add(src.caption.c_str());
    }
    sourceChoice_->value(0);
    sourceChoice_->redraw();
}

void ModuleManagerDialog::repopulateModuleBrowser() {
    if (!moduleBrowser_) return;

    moduleBrowser_->clear();
    visibleModuleRows_.clear();
    moduleBrowser_->add("Status\tModule\tType\tSource\tInstalled\tAvailable");

    std::string sourceFilter;
    const Fl_Menu_Item* sourceItem = sourceChoice_ ? sourceChoice_->mvalue() : nullptr;
    if (sourceItem && sourceItem->label()) {
        sourceFilter = sourceItem->label();
    }
    bool allSources = sourceFilter.empty() || sourceFilter == "All Sources";

    for (size_t i = 0; i < modules_.size(); ++i) {
        const ModuleRow& row = modules_[i];
        if (!allSources && row.sourceCaption != sourceFilter) continue;

        std::string line = row.statusText + "\t" +
                           row.moduleName + "\t" +
                           row.moduleType + "\t" +
                           row.sourceCaption + "\t" +
                           (row.installedVersion.empty() ? "-" : row.installedVersion) + "\t" +
                           (row.availableVersion.empty() ? "-" : row.availableVersion);
        moduleBrowser_->add(line.c_str());
        visibleModuleRows_.push_back(static_cast<int>(i));
    }

    std::string summary = std::to_string(visibleModuleRows_.size()) +
                          " modules listed";
    if (statusBox_) statusBox_->copy_label(summary.c_str());
    moduleBrowser_->redraw();
}

int ModuleManagerDialog::selectedVisibleRow() const {
    if (!moduleBrowser_) return -1;
    int line = moduleBrowser_->value();
    if (line <= 1) return -1; // header row
    int idx = line - 2;
    if (idx < 0 || idx >= static_cast<int>(visibleModuleRows_.size())) return -1;
    return visibleModuleRows_[idx];
}

bool ModuleManagerDialog::confirmRemoteNetworkUse() {
    int answer = fl_choice(
        "Remote module sources can be monitored on the network.\n\n"
        "If you live in a persecuted country and do not want to risk detection,\n"
        "do not use remote sources.\n\n"
        "Continue with remote network access?",
        "Cancel", "Continue", nullptr);
    return answer == 1;
}

void ModuleManagerDialog::addRemoteSource() {
    if (!installMgr_) initializeInstallMgr();
    if (!installMgr_) return;

    const char* captionRaw = fl_input("Remote source name:", "");
    if (!captionRaw || !*captionRaw) return;
    std::string caption = trimCopy(captionRaw);
    if (caption.empty()) return;

    const char* hostRaw = fl_input("Host (example: ftp.crosswire.org):", "");
    if (!hostRaw || !*hostRaw) return;
    std::string host = trimCopy(hostRaw);
    if (host.empty()) return;

    const char* dirRaw = fl_input("Remote directory (example: /pub/sword/raw):", "/");
    if (!dirRaw || !*dirRaw) return;
    std::string dir = trimCopy(dirRaw);

    const char* protoRaw = fl_input("Protocol (FTP/HTTP/HTTPS/SFTP):", "FTP");
    if (!protoRaw || !*protoRaw) return;
    std::string proto = upperCopy(trimCopy(protoRaw));
    if (proto != "FTP" && proto != "HTTP" &&
        proto != "HTTPS" && proto != "SFTP") {
        fl_alert("Unsupported protocol.");
        return;
    }

    auto existing = installMgr_->sources.find(caption.c_str());
    if (existing != installMgr_->sources.end()) {
        delete existing->second;
        installMgr_->sources.erase(existing);
    }

    sword::InstallSource* src = new sword::InstallSource(proto.c_str());
    src->caption = caption.c_str();
    src->source = host.c_str();
    src->directory = dir.c_str();
    installMgr_->sources[src->caption] = src;
    installMgr_->saveInstallConf();

    refreshSources(false);
    refreshModules();
}

void ModuleManagerDialog::addLocalSource() {
    std::string startDir = userHomeDir().empty() ? "." : userHomeDir();
    const char* dirRaw = fl_dir_chooser("Select local module source directory", startDir.c_str());
    if (!dirRaw || !*dirRaw) return;

    std::string dir = trimCopy(dirRaw);
    if (dir.empty()) return;

    const char* captionRaw = fl_input("Local source name:", dir.c_str());
    if (!captionRaw || !*captionRaw) return;
    std::string caption = trimCopy(captionRaw);
    if (caption.empty()) return;

    std::string confFile = installMgrPath_ + "/InstallMgr.conf";
    sword::SWConfig conf(confFile.c_str());

    // Remove any existing local source with same caption.
    auto secIt = conf.Sections.find("Sources");
    if (secIt != conf.Sections.end()) {
        for (auto it = secIt->second.begin(); it != secIt->second.end(); ) {
            if (safeText(it->first).find("DIRSource") != 0) {
                ++it;
                continue;
            }
            sword::InstallSource src("DIR", it->second.c_str());
            if (caption == safeText(src.caption)) {
                it = secIt->second.erase(it);
            } else {
                ++it;
            }
        }
    }

    sword::InstallSource local("DIR");
    local.caption = caption.c_str();
    local.source = dir.c_str();
    local.directory = dir.c_str();
    conf["Sources"].insert(std::make_pair(sword::SWBuf("DIRSource"), local.getConfEnt()));
    conf.Save();

    refreshSources(false);
    refreshModules();
}

void ModuleManagerDialog::removeSelectedSource() {
    if (!sourceChoice_) return;
    const Fl_Menu_Item* item = sourceChoice_->mvalue();
    if (!item || !item->label()) return;
    std::string caption = item->label();
    if (caption.empty() || caption == "All Sources") return;

    auto srcIt = std::find_if(sources_.begin(), sources_.end(),
                              [&](const SourceRow& s) { return s.caption == caption; });
    if (srcIt == sources_.end()) return;

    std::string removePrompt = "Remove source '" + caption + "'?";
    if (fl_choice("%s", "Cancel", "Remove", nullptr, removePrompt.c_str()) != 1) {
        return;
    }

    if (srcIt->isLocal) {
        std::string confFile = installMgrPath_ + "/InstallMgr.conf";
        sword::SWConfig conf(confFile.c_str());
        auto secIt = conf.Sections.find("Sources");
        if (secIt != conf.Sections.end()) {
            for (auto it = secIt->second.begin(); it != secIt->second.end(); ) {
                if (safeText(it->first).find("DIRSource") != 0) {
                    ++it;
                    continue;
                }
                sword::InstallSource src("DIR", it->second.c_str());
                if (caption == safeText(src.caption)) {
                    it = secIt->second.erase(it);
                } else {
                    ++it;
                }
            }
        }
        conf.Save();
    } else if (installMgr_) {
        auto it = installMgr_->sources.find(caption.c_str());
        if (it != installMgr_->sources.end()) {
            delete it->second;
            installMgr_->sources.erase(it);
            installMgr_->saveInstallConf();
        }
    }

    refreshSources(false);
    refreshModules();
}

void ModuleManagerDialog::refreshSelectedSource() {
    if (!sourceChoice_) return;
    const Fl_Menu_Item* item = sourceChoice_->mvalue();
    if (!item || !item->label()) return;
    std::string caption = item->label();
    if (caption.empty() || caption == "All Sources") {
        refreshSources(false);
        refreshModules();
        return;
    }

    auto srcIt = std::find_if(sources_.begin(), sources_.end(),
                              [&](const SourceRow& s) { return s.caption == caption; });
    if (srcIt == sources_.end()) return;

    if (!srcIt->isLocal && srcIt->remoteSource) {
        if (!confirmRemoteNetworkUse()) return;
        statusBox_->copy_label(("Refreshing " + srcIt->caption + "...").c_str());
        Fl::check();
        installMgr_->refreshRemoteSource(srcIt->remoteSource);
    }

    refreshSources(false);
    refreshModules();
}

void ModuleManagerDialog::installOrUpdateSelectedModule() {
    if (!installMgr_) initializeInstallMgr();
    if (!installMgr_) return;

    int moduleIndex = selectedVisibleRow();
    if (moduleIndex < 0 || moduleIndex >= static_cast<int>(modules_.size())) {
        fl_alert("Select a module first.");
        return;
    }

    const ModuleRow& row = modules_[moduleIndex];
    if (!row.sourcePath.empty() && row.sourceType == "DIR") {
        // Local source install.
    } else {
        if (!confirmRemoteNetworkUse()) return;
    }

    statusBox_->copy_label(("Installing " + row.moduleName + "...").c_str());
    Fl::check();

    sword::SWMgr destMgr(new sword::MarkupFilterMgr(sword::FMT_XHTML));
    int rc = 0;
    if (row.sourceType == "DIR") {
        rc = installMgr_->installModule(
            &destMgr,
            row.sourcePath.c_str(),
            row.moduleName.c_str(),
            nullptr);
    } else {
        rc = installMgr_->installModule(
            &destMgr,
            nullptr,
            row.moduleName.c_str(),
            row.remoteSource);
    }

    if (rc != 0) {
        std::string msg = "Module install failed (" + std::to_string(rc) + ").";
        fl_alert("%s", msg.c_str());
        statusBox_->copy_label("Install failed.");
        return;
    }

    // Reload installed module registry and UI panes.
    app_->swordManager().initialize();
    if (app_->mainWindow()) {
        app_->mainWindow()->refresh();
    }

    // Rebuild indexed Bible module content when needed.
    if (row.isBible && app_->searchIndexer()) {
        app_->searchIndexer()->queueModuleIndex(row.moduleName, true);
    }

    refreshSources(false);
    refreshModules();

    std::string done = "Installed/updated " + row.moduleName;
    statusBox_->copy_label(done.c_str());
}

void ModuleManagerDialog::onSourceChanged(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->repopulateModuleBrowser();
}

void ModuleManagerDialog::onRefreshSource(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->refreshSelectedSource();
}

void ModuleManagerDialog::onRefreshAll(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    if (!self->confirmRemoteNetworkUse()) return;
    if (self->installMgr_) {
        self->installMgr_->refreshRemoteSourceConfiguration();
    }
    self->refreshSources(true);
    self->refreshModules();
}

void ModuleManagerDialog::onAddRemote(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->addRemoteSource();
}

void ModuleManagerDialog::onAddLocal(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->addLocalSource();
}

void ModuleManagerDialog::onRemoveSource(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->removeSelectedSource();
}

void ModuleManagerDialog::onInstallUpdate(Fl_Widget* /*w*/, void* data) {
    auto* self = static_cast<ModuleManagerDialog*>(data);
    if (!self) return;
    self->installOrUpdateSelectedModule();
}

} // namespace verdad
