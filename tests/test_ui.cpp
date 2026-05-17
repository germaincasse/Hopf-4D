#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"
#include "ui/UIRenderer.h"

using namespace hopf;
using Catch::Approx;

namespace {

class TestButtonReceiver : public scripting::Script {
public:
    const char* typeName() const override { return "TestButtonReceiver"; }
    void onMessage(const std::string& m) override {
        last = m;
        ++count;
    }
    static std::string last;
    static int         count;
};
std::string TestButtonReceiver::last;
int         TestButtonReceiver::count = 0;

} // namespace

HOPF_REGISTER_SCRIPT(TestButtonReceiver)

TEST_CASE("resolveAnchor: TopLeft places rect at canvas origin + offset", "[ui][anchor]") {
    float mnX, mnY, mxX, mxY;
    ui::resolveAnchor(scene::UIAnchor::TopLeft,
                      /*x*/ 50.f, /*y*/ 20.f, /*w*/ 200.f, /*h*/ 100.f,
                      /*cmX*/ 10.f, /*cmY*/ 5.f,
                      /*csX*/ 1920.f, /*csY*/ 1080.f,
                      mnX, mnY, mxX, mxY);
    // Center sits at canvasMin + offset; rect extends half-size around it.
    REQUIRE(mnX == Approx(10.f + 50.f - 100.f));
    REQUIRE(mnY == Approx(5.f  + 20.f - 50.f));
    REQUIRE(mxX == Approx(10.f + 50.f + 100.f));
    REQUIRE(mxY == Approx(5.f  + 20.f + 50.f));
}

TEST_CASE("resolveAnchor: Center places rect at canvas center", "[ui][anchor]") {
    float mnX, mnY, mxX, mxY;
    ui::resolveAnchor(scene::UIAnchor::Center,
                      0.f, 0.f, 100.f, 40.f,
                      0.f, 0.f, 1000.f, 600.f,
                      mnX, mnY, mxX, mxY);
    REQUIRE(mnX == Approx(450.f));
    REQUIRE(mnY == Approx(280.f));
    REQUIRE(mxX == Approx(550.f));
    REQUIRE(mxY == Approx(320.f));
}

TEST_CASE("resolveAnchor: BottomRight places rect at canvas bottom-right corner", "[ui][anchor]") {
    float mnX, mnY, mxX, mxY;
    ui::resolveAnchor(scene::UIAnchor::BottomRight,
                      -50.f, -20.f, 100.f, 40.f,
                      0.f, 0.f, 800.f, 600.f,
                      mnX, mnY, mxX, mxY);
    REQUIRE(mxX == Approx(800.f - 50.f + 50.f));
    REQUIRE(mxY == Approx(600.f - 20.f + 20.f));
}

TEST_CASE("dispatchMessage delivers to the first script of the named type", "[ui][dispatch]") {
    TestButtonReceiver::last  = "";
    TestButtonReceiver::count = 0;

    scene::Scene s;
    const auto eId = s.addEntity("Receiver").id;
    auto* e = s.findEntity(eId);
    scene::ScriptInstance si;
    si.typeName = "TestButtonReceiver";
    si.instance = scripting::ScriptRegistry::instance().create("TestButtonReceiver");
    e->scripts.push_back(std::move(si));

    const bool ok = ui::dispatchMessage(s, "TestButtonReceiver", "onClick");
    REQUIRE(ok);
    REQUIRE(TestButtonReceiver::last  == "onClick");
    REQUIRE(TestButtonReceiver::count == 1);
}

TEST_CASE("dispatchMessage returns false when no receiver is registered", "[ui][dispatch]") {
    scene::Scene s;
    s.addEntity("Empty");
    REQUIRE_FALSE(ui::dispatchMessage(s, "NoSuchScript", "onClick"));
}
