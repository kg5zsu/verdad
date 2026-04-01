#ifndef VERDAD_READING_PLAN_UTILS_H
#define VERDAD_READING_PLAN_UTILS_H

#include "reading/ReadingPlanManager.h"

#include <string>
#include <vector>

namespace verdad {
namespace reading {

std::string trimCopy(const std::string& text);
std::vector<std::string> splitPlanLines(const std::string& text);
std::string joinPlanPassages(const std::vector<ReadingPlanPassage>& passages,
                             const char* separator);
std::string formatReadingPlanDayLabel(const ReadingPlanDay& day);
void normalizeReadingPlanDays(std::vector<ReadingPlanDay>& days);

} // namespace reading
} // namespace verdad

#endif // VERDAD_READING_PLAN_UTILS_H
