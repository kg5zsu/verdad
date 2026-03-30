#include "reading/ReadingPlanManager.h"
#include "reading/DateUtils.h"

#include <sqlite3.h>

#include <algorithm>
#include <ctime>
#include <iostream>
#include <map>
#include <set>
#include <utility>

namespace verdad {
namespace {

bool execSql(sqlite3* db, const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: "
                  << (err ? err : sqlite3_errmsg(db))
                  << "\n";
    }
    if (err) sqlite3_free(err);
    return rc == SQLITE_OK;
}

bool bindText(sqlite3_stmt* stmt, int index, const std::string& value) {
    return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

void applyPragmas(sqlite3* db) {
    if (!db) return;
    sqlite3_busy_timeout(db, 5000);
    execSql(db, "PRAGMA foreign_keys=ON;");
    execSql(db, "PRAGMA journal_mode=WAL;");
    execSql(db, "PRAGMA synchronous=NORMAL;");
}

bool beginTransaction(sqlite3* db) {
    return execSql(db, "BEGIN IMMEDIATE TRANSACTION;");
}

bool commitTransaction(sqlite3* db) {
    return execSql(db, "COMMIT;");
}

void rollbackTransaction(sqlite3* db) {
    execSql(db, "ROLLBACK;");
}

std::vector<ReadingPlanDay> sortedDays(std::vector<ReadingPlanDay> days) {
    std::sort(days.begin(), days.end(),
              [](const ReadingPlanDay& lhs, const ReadingPlanDay& rhs) {
                  if (lhs.dateIso != rhs.dateIso) return lhs.dateIso < rhs.dateIso;
                  return lhs.id < rhs.id;
              });
    return days;
}

void mergeDayInto(std::map<std::string, ReadingPlanDay>& merged,
                  ReadingPlanDay day) {
    auto it = merged.find(day.dateIso);
    if (it == merged.end()) {
        merged.emplace(day.dateIso, std::move(day));
        return;
    }

    ReadingPlanDay& target = it->second;
    if (target.title.empty()) target.title = day.title;
    target.completed = target.completed && day.completed;
    for (auto& passage : day.passages) {
        target.passages.push_back(std::move(passage));
    }
}

} // namespace

ReadingPlanManager::ReadingPlanManager() = default;

ReadingPlanManager::~ReadingPlanManager() {
    save();
    closeDatabase();
}

bool ReadingPlanManager::load(const std::string& filepath) {
    return openDatabase(filepath);
}

bool ReadingPlanManager::save() {
    return db_ != nullptr;
}

std::vector<ReadingPlanSummary> ReadingPlanManager::listPlans() const {
    std::vector<ReadingPlanSummary> plans;
    if (!db_) return plans;

    static const char* kSql = R"SQL(
        SELECT p.id, p.name, p.description, p.color, p.start_date,
               COUNT(d.id) AS total_days,
               COALESCE(SUM(CASE WHEN d.completed != 0 THEN 1 ELSE 0 END), 0) AS completed_days
        FROM reading_plans p
        LEFT JOIN reading_plan_days d ON d.plan_id = p.id
        GROUP BY p.id, p.name, p.description, p.color, p.start_date
        ORDER BY LOWER(p.name), p.id;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) != SQLITE_OK) {
        return plans;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ReadingPlanSummary summary;
        summary.id = sqlite3_column_int(stmt, 0);
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        const char* description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        const char* color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* startDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        summary.name = name ? name : "";
        summary.description = description ? description : "";
        summary.color = color ? color : "";
        summary.startDateIso = startDate ? startDate : "";
        summary.totalDays = sqlite3_column_int(stmt, 5);
        summary.completedDays = sqlite3_column_int(stmt, 6);
        plans.push_back(std::move(summary));
    }

    sqlite3_finalize(stmt);
    return plans;
}

bool ReadingPlanManager::getPlan(int planId, ReadingPlan& out) const {
    if (!db_) return false;
    return loadSinglePlan(db_, planId, out);
}

bool ReadingPlanManager::createPlan(const ReadingPlan& plan, int* createdId) {
    if (!db_ || plan.summary.name.empty()) return false;

    if (!beginTransaction(db_)) return false;

    static const char* kInsertSql = R"SQL(
        INSERT INTO reading_plans (name, description, color, start_date)
        VALUES (?, ?, ?, ?);
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    bool ok = sqlite3_prepare_v2(db_, kInsertSql, -1, &stmt, nullptr) == SQLITE_OK;
    if (ok) ok = bindText(stmt, 1, plan.summary.name);
    if (ok) ok = bindText(stmt, 2, plan.summary.description);
    if (ok) ok = bindText(stmt, 3, plan.summary.color);
    if (ok) ok = bindText(stmt, 4, plan.summary.startDateIso);
    if (ok) ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (stmt) sqlite3_finalize(stmt);

    int planId = ok ? static_cast<int>(sqlite3_last_insert_rowid(db_)) : 0;
    if (ok) {
        ok = replacePlanDays(db_, planId, sortedDays(plan.days));
    }

    if (ok) {
        ok = commitTransaction(db_);
    } else {
        rollbackTransaction(db_);
    }

    if (ok && createdId) *createdId = planId;
    return ok;
}

bool ReadingPlanManager::updatePlan(const ReadingPlan& plan) {
    if (!db_ || plan.summary.id <= 0 || plan.summary.name.empty()) return false;

    if (!beginTransaction(db_)) return false;

    static const char* kUpdateSql = R"SQL(
        UPDATE reading_plans
           SET name = ?, description = ?, color = ?, start_date = ?
         WHERE id = ?;
    )SQL";

    sqlite3_stmt* stmt = nullptr;
    bool ok = sqlite3_prepare_v2(db_, kUpdateSql, -1, &stmt, nullptr) == SQLITE_OK;
    if (ok) ok = bindText(stmt, 1, plan.summary.name);
    if (ok) ok = bindText(stmt, 2, plan.summary.description);
    if (ok) ok = bindText(stmt, 3, plan.summary.color);
    if (ok) ok = bindText(stmt, 4, plan.summary.startDateIso);
    if (ok) ok = sqlite3_bind_int(stmt, 5, plan.summary.id) == SQLITE_OK;
    if (ok) ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    if (stmt) sqlite3_finalize(stmt);

    if (ok) {
        ok = replacePlanDays(db_, plan.summary.id, sortedDays(plan.days));
    }

    if (ok) {
        ok = commitTransaction(db_);
    } else {
        rollbackTransaction(db_);
    }
    return ok;
}

bool ReadingPlanManager::deletePlan(int planId) {
    if (!db_ || planId <= 0) return false;

    static const char* kSql = "DELETE FROM reading_plans WHERE id = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool ok = sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_bind_int(stmt, 1, planId) == SQLITE_OK;
    if (ok) ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    if (stmt) sqlite3_finalize(stmt);
    return ok;
}

bool ReadingPlanManager::setDayCompleted(int planId,
                                         const std::string& dateIso,
                                         bool completed) {
    if (!db_ || planId <= 0 || !reading::isIsoDateInRange(dateIso)) return false;

    static const char* kSql =
        "UPDATE reading_plan_days SET completed = ? WHERE plan_id = ? AND day_date = ?;";
    sqlite3_stmt* stmt = nullptr;
    bool ok = sqlite3_prepare_v2(db_, kSql, -1, &stmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_bind_int(stmt, 1, completed ? 1 : 0) == SQLITE_OK;
    if (ok) ok = sqlite3_bind_int(stmt, 2, planId) == SQLITE_OK;
    if (ok) ok = bindText(stmt, 3, dateIso);
    if (ok) ok = sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db_) > 0;
    if (stmt) sqlite3_finalize(stmt);
    return ok;
}

bool ReadingPlanManager::rescheduleDay(int planId,
                                       const std::string& fromDateIso,
                                       const std::string& toDateIso,
                                       bool shiftLaterIncompleteDays) {
    if (!db_ || planId <= 0 ||
        !reading::isIsoDateInRange(fromDateIso) ||
        !reading::isIsoDateInRange(toDateIso)) {
        return false;
    }

    ReadingPlan plan;
    if (!getPlan(planId, plan)) return false;

    reading::Date fromDate{};
    reading::Date toDate{};
    reading::parseIsoDate(fromDateIso, fromDate);
    reading::parseIsoDate(toDateIso, toDate);
    std::tm toTm = reading::toTm(toDate);
    std::tm fromTm = reading::toTm(fromDate);
    const int dayDelta = static_cast<int>(
        std::difftime(std::mktime(&toTm),
                      std::mktime(&fromTm)) / (60 * 60 * 24));

    std::map<std::string, ReadingPlanDay> merged;
    bool movedAny = false;

    for (auto day : plan.days) {
        if (day.dateIso == fromDateIso && !shiftLaterIncompleteDays) {
            day.dateIso = toDateIso;
            movedAny = true;
        } else if (shiftLaterIncompleteDays && !day.completed) {
            reading::Date current{};
            if (reading::parseIsoDate(day.dateIso, current) &&
                reading::compareDates(current, fromDate) >= 0) {
                day.dateIso = reading::formatIsoDate(reading::addDays(current, dayDelta));
                movedAny = true;
            }
        }
        mergeDayInto(merged, std::move(day));
    }

    if (!movedAny) return false;

    plan.days.clear();
    for (auto& entry : merged) {
        plan.days.push_back(std::move(entry.second));
    }
    return updatePlan(plan);
}

bool ReadingPlanManager::openDatabase(const std::string& filepath) {
    if (filepath.empty()) return false;
    if (db_ && filepath_ == filepath) return true;

    closeDatabase();

    sqlite3* newDb = nullptr;
    int rc = sqlite3_open_v2(filepath.c_str(),
                             &newDb,
                             SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                             nullptr);
    if (rc != SQLITE_OK || !newDb) {
        if (newDb) sqlite3_close(newDb);
        return false;
    }

    db_ = newDb;
    filepath_ = filepath;
    applyPragmas(db_);
    if (!ensureSchema()) {
        closeDatabase();
        return false;
    }
    return true;
}

void ReadingPlanManager::closeDatabase() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
    filepath_.clear();
}

bool ReadingPlanManager::ensureSchema() {
    if (!db_) return false;

    static const char* kSchemaSql = R"SQL(
        CREATE TABLE IF NOT EXISTS reading_plans (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            description TEXT NOT NULL DEFAULT '',
            color TEXT NOT NULL DEFAULT '',
            start_date TEXT NOT NULL DEFAULT ''
        );

        CREATE TABLE IF NOT EXISTS reading_plan_days (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            plan_id INTEGER NOT NULL,
            day_date TEXT NOT NULL,
            title TEXT NOT NULL DEFAULT '',
            completed INTEGER NOT NULL DEFAULT 0,
            FOREIGN KEY (plan_id) REFERENCES reading_plans(id)
                ON DELETE CASCADE
                ON UPDATE CASCADE,
            UNIQUE(plan_id, day_date)
        );

        CREATE TABLE IF NOT EXISTS reading_plan_day_passages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            day_id INTEGER NOT NULL,
            sort_order INTEGER NOT NULL DEFAULT 0,
            reference TEXT NOT NULL,
            FOREIGN KEY (day_id) REFERENCES reading_plan_days(id)
                ON DELETE CASCADE
                ON UPDATE CASCADE
        );

        CREATE INDEX IF NOT EXISTS idx_reading_plan_days_plan_date
            ON reading_plan_days(plan_id, day_date);

        CREATE INDEX IF NOT EXISTS idx_reading_plan_day_passages_day_sort
            ON reading_plan_day_passages(day_id, sort_order);

        PRAGMA user_version = 1;
    )SQL";

    return execSql(db_, kSchemaSql);
}

bool ReadingPlanManager::loadPlanDays(sqlite3* db,
                                      int planId,
                                      std::vector<ReadingPlanDay>& out) const {
    if (!db || planId <= 0) return false;

    static const char* kDaySql = R"SQL(
        SELECT id, day_date, title, completed
          FROM reading_plan_days
         WHERE plan_id = ?
         ORDER BY day_date, id;
    )SQL";
    static const char* kPassageSql = R"SQL(
        SELECT id, reference
          FROM reading_plan_day_passages
         WHERE day_id = ?
         ORDER BY sort_order, id;
    )SQL";

    sqlite3_stmt* dayStmt = nullptr;
    sqlite3_stmt* passageStmt = nullptr;
    bool ok = sqlite3_prepare_v2(db, kDaySql, -1, &dayStmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_prepare_v2(db, kPassageSql, -1, &passageStmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_bind_int(dayStmt, 1, planId) == SQLITE_OK;

    while (ok && sqlite3_step(dayStmt) == SQLITE_ROW) {
        ReadingPlanDay day;
        day.id = sqlite3_column_int(dayStmt, 0);
        const char* dateIso = reinterpret_cast<const char*>(sqlite3_column_text(dayStmt, 1));
        const char* title = reinterpret_cast<const char*>(sqlite3_column_text(dayStmt, 2));
        day.dateIso = dateIso ? dateIso : "";
        day.title = title ? title : "";
        day.completed = sqlite3_column_int(dayStmt, 3) != 0;

        sqlite3_reset(passageStmt);
        sqlite3_clear_bindings(passageStmt);
        ok = sqlite3_bind_int(passageStmt, 1, day.id) == SQLITE_OK;
        while (ok && sqlite3_step(passageStmt) == SQLITE_ROW) {
            ReadingPlanPassage passage;
            passage.id = sqlite3_column_int(passageStmt, 0);
            const char* ref = reinterpret_cast<const char*>(sqlite3_column_text(passageStmt, 1));
            passage.reference = ref ? ref : "";
            day.passages.push_back(std::move(passage));
        }
        if (ok) {
            out.push_back(std::move(day));
        }
    }

    if (dayStmt) sqlite3_finalize(dayStmt);
    if (passageStmt) sqlite3_finalize(passageStmt);
    return ok;
}

bool ReadingPlanManager::replacePlanDays(sqlite3* db,
                                         int planId,
                                         const std::vector<ReadingPlanDay>& days) const {
    if (!db || planId <= 0) return false;

    static const char* kDeleteSql =
        "DELETE FROM reading_plan_days WHERE plan_id = ?;";
    static const char* kInsertDaySql = R"SQL(
        INSERT INTO reading_plan_days (plan_id, day_date, title, completed)
        VALUES (?, ?, ?, ?);
    )SQL";
    static const char* kInsertPassageSql = R"SQL(
        INSERT INTO reading_plan_day_passages (day_id, sort_order, reference)
        VALUES (?, ?, ?);
    )SQL";

    sqlite3_stmt* deleteStmt = nullptr;
    sqlite3_stmt* dayStmt = nullptr;
    sqlite3_stmt* passageStmt = nullptr;
    bool ok = sqlite3_prepare_v2(db, kDeleteSql, -1, &deleteStmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_prepare_v2(db, kInsertDaySql, -1, &dayStmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_prepare_v2(db, kInsertPassageSql, -1, &passageStmt, nullptr) == SQLITE_OK;

    if (ok) ok = sqlite3_bind_int(deleteStmt, 1, planId) == SQLITE_OK;
    if (ok) ok = sqlite3_step(deleteStmt) == SQLITE_DONE;

    std::map<std::string, ReadingPlanDay> merged;
    for (auto day : days) {
        day.id = 0;
        mergeDayInto(merged, std::move(day));
    }

    for (const auto& entry : merged) {
        const ReadingPlanDay& day = entry.second;
        if (!ok) break;
        if (!reading::isIsoDateInRange(day.dateIso)) {
            ok = false;
            break;
        }

        sqlite3_reset(dayStmt);
        sqlite3_clear_bindings(dayStmt);
        ok = sqlite3_bind_int(dayStmt, 1, planId) == SQLITE_OK;
        if (ok) ok = bindText(dayStmt, 2, day.dateIso);
        if (ok) ok = bindText(dayStmt, 3, day.title);
        if (ok) ok = sqlite3_bind_int(dayStmt, 4, day.completed ? 1 : 0) == SQLITE_OK;
        if (ok) ok = sqlite3_step(dayStmt) == SQLITE_DONE;
        int dayId = ok ? static_cast<int>(sqlite3_last_insert_rowid(db)) : 0;

        for (size_t i = 0; ok && i < day.passages.size(); ++i) {
            sqlite3_reset(passageStmt);
            sqlite3_clear_bindings(passageStmt);
            ok = sqlite3_bind_int(passageStmt, 1, dayId) == SQLITE_OK;
            if (ok) ok = sqlite3_bind_int(passageStmt, 2, static_cast<int>(i)) == SQLITE_OK;
            if (ok) ok = bindText(passageStmt, 3, day.passages[i].reference);
            if (ok) ok = sqlite3_step(passageStmt) == SQLITE_DONE;
        }
    }

    if (deleteStmt) sqlite3_finalize(deleteStmt);
    if (dayStmt) sqlite3_finalize(dayStmt);
    if (passageStmt) sqlite3_finalize(passageStmt);
    return ok;
}

bool ReadingPlanManager::loadSinglePlan(sqlite3* db,
                                        int planId,
                                        ReadingPlan& out) const {
    if (!db || planId <= 0) return false;

    static const char* kSql =
        "SELECT id, name, description, color, start_date FROM reading_plans WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    bool ok = sqlite3_prepare_v2(db, kSql, -1, &stmt, nullptr) == SQLITE_OK;
    if (ok) ok = sqlite3_bind_int(stmt, 1, planId) == SQLITE_OK;
    if (!ok) {
        if (stmt) sqlite3_finalize(stmt);
        return false;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return false;
    }

    const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    const char* color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    const char* startDate = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    out.summary.id = sqlite3_column_int(stmt, 0);
    out.summary.name = name ? name : "";
    out.summary.description = description ? description : "";
    out.summary.color = color ? color : "";
    out.summary.startDateIso = startDate ? startDate : "";
    sqlite3_finalize(stmt);

    out.days.clear();
    ok = loadPlanDays(db, planId, out.days);
    if (ok) {
        out.summary.totalDays = static_cast<int>(out.days.size());
        out.summary.completedDays = static_cast<int>(
            std::count_if(out.days.begin(), out.days.end(),
                          [](const ReadingPlanDay& day) { return day.completed; }));
    }
    return ok;
}

} // namespace verdad
