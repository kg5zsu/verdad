#ifndef VERDAD_DAILY_WORKSPACE_STATE_H
#define VERDAD_DAILY_WORKSPACE_STATE_H

#include <string>

namespace verdad {

enum class DailyWorkspaceMode {
    Devotionals,
    ReadingPlans,
};

struct DailyWorkspaceState {
    bool tabActive = false;
    DailyWorkspaceMode mode = DailyWorkspaceMode::Devotionals;
    std::string devotionalModule;
    int readingPlanId = 0;
    std::string selectedDateIso;
    bool calendarVisible = false;
};

inline const char* dailyWorkspaceModeToken(DailyWorkspaceMode mode) {
    switch (mode) {
    case DailyWorkspaceMode::Devotionals:
        return "devotionals";
    case DailyWorkspaceMode::ReadingPlans:
        return "reading_plans";
    }
    return "devotionals";
}

inline DailyWorkspaceMode dailyWorkspaceModeFromToken(const std::string& text) {
    if (text == "reading_plans" || text == "plans") {
        return DailyWorkspaceMode::ReadingPlans;
    }
    return DailyWorkspaceMode::Devotionals;
}

} // namespace verdad

#endif // VERDAD_DAILY_WORKSPACE_STATE_H
