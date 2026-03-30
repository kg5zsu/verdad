#ifndef VERDAD_READING_PLAN_EDITOR_DIALOG_H
#define VERDAD_READING_PLAN_EDITOR_DIALOG_H

#include "reading/ReadingPlanManager.h"

namespace verdad {

class ReadingPlanEditorDialog {
public:
    static bool editPlan(ReadingPlan& plan, bool creating);
};

} // namespace verdad

#endif // VERDAD_READING_PLAN_EDITOR_DIALOG_H
