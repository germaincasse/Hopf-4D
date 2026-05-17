#pragma once

#include "scripting/Script.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace hopf::scripting {

// Process-wide registry of available script types. Users register their script
// classes via HOPF_REGISTER_SCRIPT(MyScript) at file scope; the editor then sees
// "MyScript" in the Inspector's Add Script popup.
class ScriptRegistry {
public:
    using Factory = std::function<std::unique_ptr<Script>()>;

    static ScriptRegistry& instance();

    void registerScript(std::string name, Factory factory);

    // Returns nullptr if the name isn't registered (e.g. removed from the binary).
    std::unique_ptr<Script> create(const std::string& name) const;

    // Alphabetically sorted list of registered names, suitable for UI display.
    std::vector<std::string> names() const;

private:
    struct Entry {
        std::string name;
        Factory     factory;
    };
    std::vector<Entry> m_entries;
};

} // namespace hopf::scripting

// Place at file scope to register a Script subclass. The class must be default
// constructible.
#define HOPF_REGISTER_SCRIPT(Type)                                                   \
    namespace {                                                                      \
    struct Type##_HopfRegistrar {                                                    \
        Type##_HopfRegistrar() {                                                     \
            ::hopf::scripting::ScriptRegistry::instance().registerScript(            \
                #Type,                                                               \
                []() -> std::unique_ptr<::hopf::scripting::Script> {                 \
                    return std::make_unique<Type>();                                 \
                });                                                                  \
        }                                                                            \
    };                                                                                \
    static Type##_HopfRegistrar s_##Type##_HopfRegistrar;                            \
    }
