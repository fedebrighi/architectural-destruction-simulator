#include "Application.h"
#include <iostream>
#include <exception>
#include <glm/glm.hpp>

int main() {
    try {
        Application app;

        if (!app.initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }

        app.run();

    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown application error occurred" << std::endl;
        return -1;
    }

    std::cout << "Application exited successfully" << std::endl;
    return 0;
}