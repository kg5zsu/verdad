#ifndef VERDAD_READING_DATE_UTILS_H
#define VERDAD_READING_DATE_UTILS_H

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>

namespace verdad {
namespace reading {

struct Date {
    int year = 0;
    int month = 0;
    int day = 0;

    bool valid() const {
        return year > 0 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
    }
};

inline bool parseIsoDate(const std::string& text, Date& out) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
    try {
        out.year = std::stoi(text.substr(0, 4));
        out.month = std::stoi(text.substr(5, 2));
        out.day = std::stoi(text.substr(8, 2));
    } catch (...) {
        return false;
    }
    return out.valid();
}

inline std::string formatIsoDate(const Date& date) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
                  date.year, date.month, date.day);
    return std::string(buffer);
}

inline std::tm toTm(const Date& date) {
    std::tm value{};
    value.tm_year = date.year - 1900;
    value.tm_mon = date.month - 1;
    value.tm_mday = date.day;
    value.tm_hour = 12;
    value.tm_isdst = -1;
    return value;
}

inline Date fromTm(const std::tm& value) {
    return Date{value.tm_year + 1900, value.tm_mon + 1, value.tm_mday};
}

inline Date normalizeDate(const Date& date) {
    std::tm value = toTm(date);
    std::mktime(&value);
    return fromTm(value);
}

inline Date today() {
    std::time_t now = std::time(nullptr);
    std::tm localTime{};
#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    return fromTm(localTime);
}

inline Date addDays(const Date& date, int deltaDays) {
    std::tm value = toTm(date);
    value.tm_mday += deltaDays;
    std::mktime(&value);
    return fromTm(value);
}

inline Date addMonths(const Date& date, int deltaMonths) {
    std::tm value = toTm(date);
    value.tm_mon += deltaMonths;
    value.tm_mday = 1;
    std::mktime(&value);
    return fromTm(value);
}

inline int daysInMonth(int year, int month) {
    Date first{year, month, 1};
    Date next = addMonths(first, 1);
    return addDays(next, -1).day;
}

inline int weekdaySundayFirst(const Date& date) {
    std::tm value = toTm(date);
    std::mktime(&value);
    return value.tm_wday;
}

inline bool sameDate(const Date& lhs, const Date& rhs) {
    return lhs.year == rhs.year && lhs.month == rhs.month && lhs.day == rhs.day;
}

inline int compareDates(const Date& lhs, const Date& rhs) {
    if (lhs.year != rhs.year) return lhs.year < rhs.year ? -1 : 1;
    if (lhs.month != rhs.month) return lhs.month < rhs.month ? -1 : 1;
    if (lhs.day != rhs.day) return lhs.day < rhs.day ? -1 : 1;
    return 0;
}

inline bool isIsoDateInRange(const std::string& text) {
    Date date;
    return parseIsoDate(text, date);
}

inline std::string monthName(int month) {
    static const char* kMonths[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    if (month < 1 || month > 12) return "";
    return kMonths[month - 1];
}

inline std::string formatLongDate(const Date& date) {
    std::string month = monthName(date.month);
    if (month.empty()) return formatIsoDate(date);
    return month + " " + std::to_string(date.day) + ", " + std::to_string(date.year);
}

inline std::string addEllipsis(const std::string& text, size_t limit) {
    if (text.size() <= limit) return text;
    if (limit <= 3) return text.substr(0, limit);
    return text.substr(0, limit - 3) + "...";
}

} // namespace reading
} // namespace verdad

#endif // VERDAD_READING_DATE_UTILS_H
