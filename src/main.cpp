#include "core/Application.h"
#include "core/Logger.h"

#include <cstdio>
#include <exception>

int main(int /*argc*/, char** /*argv*/) {
    try {
        hopf::core::Application app;
        return app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "Fatal: %s\n", e.what());
        hopf::core::Logger::error("Fatal: %s", e.what());
        return 1;
    } catch (...) {
        std::fprintf(stderr, "Fatal: unknown exception\n");
        return 1;
    }
}
