#include "scripting/ScriptRegistry.h"

#include <algorithm>

namespace hopf::scripting {

ScriptRegistry& ScriptRegistry::instance() {
    static ScriptRegistry s;
    return s;
}

void ScriptRegistry::registerScript(std::string name, Factory factory) {
    for (auto& e : m_entries) {
        if (e.name == name) { e.factory = std::move(factory); return; }
    }
    m_entries.push_back({std::move(name), std::move(factory)});
}

std::unique_ptr<Script> ScriptRegistry::create(const std::string& name) const {
    for (const auto& e : m_entries) {
        if (e.name == name) return e.factory();
    }
    return nullptr;
}

std::vector<std::string> ScriptRegistry::names() const {
    std::vector<std::string> out;
    out.reserve(m_entries.size());
    for (const auto& e : m_entries) out.push_back(e.name);
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace hopf::scripting
