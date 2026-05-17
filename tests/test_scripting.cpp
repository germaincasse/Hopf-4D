#include <catch2/catch_test_macros.hpp>

#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

namespace {

// A trivial script type that counts onUpdate calls into a static counter so
// tests can observe lifecycle.
class TickerScript : public hopf::scripting::Script {
public:
    const char* typeName() const override { return "TickerScript"; }
    void onStart() override { ++s_startCalls; }
    void onUpdate(float /*dt*/) override { ++s_updateCalls; }

    static int s_startCalls;
    static int s_updateCalls;
};
int TickerScript::s_startCalls  = 0;
int TickerScript::s_updateCalls = 0;

} // namespace

HOPF_REGISTER_SCRIPT(TickerScript)

using namespace hopf;

TEST_CASE("ScriptRegistry creates instances by name", "[scripting]") {
    auto& reg = scripting::ScriptRegistry::instance();
    auto inst = reg.create("TickerScript");
    REQUIRE(inst != nullptr);
    REQUIRE(std::string(inst->typeName()) == "TickerScript");
}

TEST_CASE("ScriptRegistry returns nullptr for unknown names", "[scripting]") {
    auto& reg = scripting::ScriptRegistry::instance();
    REQUIRE(reg.create("NonExistent") == nullptr);
}

TEST_CASE("ScriptRegistry::names lists registered types", "[scripting]") {
    auto& reg = scripting::ScriptRegistry::instance();
    const auto names = reg.names();
    bool found = false;
    for (const auto& n : names) if (n == "TickerScript") { found = true; break; }
    REQUIRE(found);
}

TEST_CASE("Entity copy re-instantiates scripts via the registry", "[scripting][entity]") {
    scene::Scene s;
    const auto eId = s.addEntity("E").id;
    {
        auto* e = s.findEntity(eId);
        scene::ScriptInstance si;
        si.typeName = "TickerScript";
        si.instance = scripting::ScriptRegistry::instance().create("TickerScript");
        REQUIRE(si.instance != nullptr);
        e->scripts.push_back(std::move(si));
    }

    // Duplicate must produce a fresh, independent instance.
    const auto dupId = s.duplicateEntity(*s.findEntity(eId), "E copy").id;

    // Re-lookup after a push_back that may have reallocated the entities vector.
    auto* e   = s.findEntity(eId);
    auto* dup = s.findEntity(dupId);
    REQUIRE(e != nullptr);
    REQUIRE(dup != nullptr);
    REQUIRE(dup->scripts.size() == 1);
    REQUIRE(dup->scripts[0].typeName == "TickerScript");
    REQUIRE(dup->scripts[0].instance != nullptr);
    REQUIRE(dup->scripts[0].instance.get() != e->scripts[0].instance.get());
}
