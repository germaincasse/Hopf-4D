#include "core/Application.h"
#include "core/Logger.h"

#include <cstdio>
#include <exception>
#include <filesystem>

int main(int argc, char** argv) {
    try {
        std::filesystem::path exePath;
        if (argc > 0 && argv[0]) exePath = argv[0];
        hopf::core::Application app(std::move(exePath));
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
