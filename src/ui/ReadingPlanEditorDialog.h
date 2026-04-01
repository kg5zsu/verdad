#ifndef VERDAD_READING_PLAN_EDITOR_DIALOG_H
#define VERDAD_READING_PLAN_EDITOR_DIALOG_H

#include "reading/ReadingPlanManager.h"

namespace verdad {

class VerdadApp;

class ReadingPlanEditorDialog {
public:
    static bool createPlan(VerdadApp* app, ReadingPlan& plan);
};

} // namespace verdad

#endif // VERDAD_READING_PLAN_EDITOR_DIALOG_H
