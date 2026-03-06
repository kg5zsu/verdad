#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Spinner.H>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <sstream>
#include <string>
#include <thread>

namespace {

constexpr double kDebounceDelaySec = 0.08;

enum class UpdateMode {
    BrowserOnly = 0,
    ImmediatePreview = 1,
    DebouncedPreview = 2,
    ImmediatePreviewWithDelay = 3,
};

class BrowserRedrawTestWindow : public Fl_Double_Window {
public:
    BrowserRedrawTestWindow(int W, int H, const char* title)
        : Fl_Double_Window(W, H, title) {
        begin();

        const int pad = 10;
        const int choiceH = 28;
        const int topRowH = 28;
        const int statusH = 28;
        const int contentY = pad + topRowH + pad;
        const int contentH = H - contentY - statusH - (pad * 2);
        const int browserW = (W - (pad * 3)) / 2;
        const int previewW = W - browserW - (pad * 3);

        modeLabel_ = new Fl_Box(pad, pad, 52, choiceH, "Mode:");
        modeChoice_ = new Fl_Choice(pad + 54, pad, 210, choiceH);
        modeChoice_->add("Browser only");
        modeChoice_->add("Immediate preview");
        modeChoice_->add("Debounced preview");
        modeChoice_->add("Immediate preview + delay");
        modeChoice_->value(static_cast<int>(UpdateMode::BrowserOnly));
        modeChoice_->callback(onModeChange, this);

        delayLabel_ = new Fl_Box(pad + 272, pad, 70, choiceH, "Delay ms:");
        delaySpinner_ = new Fl_Spinner(pad + 344, pad, 72, choiceH);
        delaySpinner_->range(0, 500);
        delaySpinner_->step(5);
        delaySpinner_->value(35);
        delaySpinner_->callback(onModeChange, this);

        backendBox_ = new Fl_Box(pad + 430, pad, W - (pad + 430) - pad, choiceH);
        backendBox_->align(FL_ALIGN_RIGHT | FL_ALIGN_INSIDE);

        browser_ = new Fl_Hold_Browser(pad, contentY, browserW, contentH);
        browser_->when(FL_WHEN_CHANGED);
        browser_->callback(onBrowserSelect, this);
        static int widths[] = { 96, 0 };
        browser_->column_widths(widths);

        preview_ = new Fl_Multiline_Output(pad + browserW + pad, contentY,
                                           previewW, contentH);
        preview_->value("Select a row and use Up/Down.\n"
                        "If Browser only flickers, that points to FLTK/backend.\n"
                        "If only preview modes flicker, it points to callback workload.");

        statusBox_ = new Fl_Box(pad, H - statusH - pad, W - (pad * 2), statusH);
        statusBox_->box(FL_DOWN_BOX);
        statusBox_->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

        end();
        resizable(browser_);

        populateRows();
        updateBackendLabel();
        updateStatus();

        if (browser_->size() > 0) {
            browser_->value(1);
            browser_->take_focus();
        }
    }

    ~BrowserRedrawTestWindow() override {
        cancelDeferredPreview();
    }

private:
    Fl_Box* modeLabel_ = nullptr;
    Fl_Choice* modeChoice_ = nullptr;
    Fl_Box* delayLabel_ = nullptr;
    Fl_Spinner* delaySpinner_ = nullptr;
    Fl_Box* backendBox_ = nullptr;
    Fl_Hold_Browser* browser_ = nullptr;
    Fl_Multiline_Output* preview_ = nullptr;
    Fl_Box* statusBox_ = nullptr;

    bool deferredPreviewScheduled_ = false;
    int pendingPreviewIndex_ = 0;
    std::string previewStorage_;
    std::string statusStorage_;
    std::string backendStorage_;

    static void onBrowserSelect(Fl_Widget*, void* data) {
        auto* self = static_cast<BrowserRedrawTestWindow*>(data);
        if (!self) return;
        self->handleBrowserSelection();
    }

    static void onModeChange(Fl_Widget*, void* data) {
        auto* self = static_cast<BrowserRedrawTestWindow*>(data);
        if (!self) return;
        self->updateStatus();
    }

    static void onDeferredPreview(void* data) {
        auto* self = static_cast<BrowserRedrawTestWindow*>(data);
        if (!self) return;
        self->deferredPreviewScheduled_ = false;
        self->applyPreview(self->pendingPreviewIndex_);
    }

    void populateRows() {
        if (!browser_) return;

        static const char* snippets[] = {
            "In the beginning God created the heaven and the earth.",
            "And the earth was without form, and void; and darkness was upon the face of the deep.",
            "And God said, Let there be light: and there was light.",
            "Blessed is the man that walketh not in the counsel of the ungodly.",
            "Trust in the LORD with all thine heart; and lean not unto thine own understanding.",
            "For God so loved the world, that he gave his only begotten Son.",
            "The LORD is my shepherd; I shall not want.",
            "Be strong and of a good courage; be not afraid, neither be thou dismayed.",
        };

        for (int i = 0; i < 500; ++i) {
            int book = (i % 8) + 1;
            int chapter = ((i / 8) % 50) + 1;
            int verse = (i % 25) + 1;

            std::ostringstream row;
            row << "Bk" << book << " " << chapter << ":" << verse
                << "\t" << snippets[i % 8];
            browser_->add(row.str().c_str());
        }
    }

    void updateBackendLabel() {
        const char* backend = std::getenv("FLTK_BACKEND");
        backendStorage_ = "FLTK_BACKEND=";
        backendStorage_ += backend ? backend : "(default)";
        if (backendBox_) backendBox_->copy_label(backendStorage_.c_str());
    }

    void updateStatus() {
        if (!modeChoice_ || !statusBox_) return;

        std::ostringstream status;
        status << "Click the left list and hold Up/Down. Current mode: "
               << modeChoice_->text(modeChoice_->value());
        if (currentMode() == UpdateMode::ImmediatePreviewWithDelay) {
            status << " (" << currentDelayMs() << " ms simulated work)";
        }

        statusStorage_ = status.str();
        statusBox_->copy_label(statusStorage_.c_str());
    }

    UpdateMode currentMode() const {
        if (!modeChoice_) return UpdateMode::BrowserOnly;
        int raw = modeChoice_->value();
        raw = std::clamp(raw, 0, 3);
        return static_cast<UpdateMode>(raw);
    }

    int currentDelayMs() const {
        if (!delaySpinner_) return 0;
        return static_cast<int>(delaySpinner_->value());
    }

    void cancelDeferredPreview() {
        if (!deferredPreviewScheduled_) return;
        Fl::remove_timeout(onDeferredPreview, this);
        deferredPreviewScheduled_ = false;
    }

    void handleBrowserSelection() {
        if (!browser_) return;

        int idx = browser_->value();
        if (idx <= 0) return;

        switch (currentMode()) {
        case UpdateMode::BrowserOnly:
            cancelDeferredPreview();
            return;

        case UpdateMode::ImmediatePreview:
            cancelDeferredPreview();
            applyPreview(idx);
            return;

        case UpdateMode::DebouncedPreview:
            pendingPreviewIndex_ = idx;
            if (deferredPreviewScheduled_) {
                Fl::remove_timeout(onDeferredPreview, this);
            }
            deferredPreviewScheduled_ = true;
            Fl::add_timeout(kDebounceDelaySec, onDeferredPreview, this);
            return;

        case UpdateMode::ImmediatePreviewWithDelay:
            cancelDeferredPreview();
            if (currentDelayMs() > 0) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(currentDelayMs()));
            }
            applyPreview(idx);
            return;
        }
    }

    void applyPreview(int idx) {
        if (!preview_ || idx <= 0) return;

        std::ostringstream text;
        text << "Selected row: " << idx << "\n";
        text << "Mode: " << modeChoice_->text(modeChoice_->value()) << "\n";
        text << "Backend: " << backendStorage_ << "\n\n";
        text << "Arrow-key redraw test\n";
        text << "---------------------\n";
        text << "If the list on the left turns gray or flashes while moving selection,\n";
        text << "the problem is reproducible in standalone FLTK.\n\n";
        text << "Current row text:\n";
        const char* line = browser_->text(idx);
        text << (line ? line : "(null)") << "\n";

        previewStorage_ = text.str();
        preview_->value(previewStorage_.c_str());
    }
};

} // namespace

int main(int argc, char** argv) {
    Fl::scheme("gtk+");

    BrowserRedrawTestWindow window(980, 620, "FLTK Browser Redraw Test");
    window.show(argc, argv);
    return Fl::run();
}
