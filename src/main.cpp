#include "app/VerdadApp.h"

#include <FL/Fl.H>
#include <FL/Fl_File_Icon.H>

#include <iostream>
#include <cstdlib>


int main(int argc, char* argv[]) {
    verdad::VerdadApp app;
//    Fl_File_Icon::load_system_icons();

    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize Verdad." << std::endl;
        return EXIT_FAILURE;
    }

    return app.run();
}
