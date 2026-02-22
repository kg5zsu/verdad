#include "app/VerdadApp.h"

#include <FL/Fl.H>

#include <iostream>
#include <cstdlib>

int main(int argc, char* argv[]) {
    verdad::VerdadApp app;

    if (!app.initialize(argc, argv)) {
        std::cerr << "Failed to initialize Verdad." << std::endl;
        return EXIT_FAILURE;
    }

    return app.run();
}
