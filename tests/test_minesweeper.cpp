#include <catch2/catch_test_macros.hpp>

#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

using namespace hopf;

namespace {

// Helper: build a fresh scene with a root entity carrying the Minesweeper4D script,
// bind it, and return both ids. Cells are spawned by onStart().
struct Fixture {
    scene::Scene             scene;
    scene::EntityId          rootId   = 0;
    scripting::Script*       script   = nullptr;
};

Fixture makeFixture() {
    Fixture f;
    f.rootId = f.scene.addEntity("Minesweeper").id;
    auto* root = f.scene.findEntity(f.rootId);
    scene::ScriptInstance si;
    si.typeName = "Minesweeper4D";
    si.instance = scripting::ScriptRegistry::instance().create("Minesweeper4D");
    REQUIRE(si.instance != nullptr);
    root->scripts.push_back(std::move(si));
    f.script = f.scene.findEntity(f.rootId)->scripts[0].instance.get();
    f.script->_bind(f.rootId, &f.scene);
    return f;
}

} // namespace

TEST_CASE("Minesweeper4D::onStart spawns a 5^4 grid of child cells", "[example][minesweeper]") {
    auto f = makeFixture();
    f.script->onStart();
    int kids = 0;
    for (const auto& e : f.scene.entities()) {
        if (e.parent == f.rootId && e.mesh != nullptr) ++kids;
    }
    REQUIRE(kids == 5 * 5 * 5 * 5);
}

TEST_CASE("Minesweeper4D first click never lands on a mine", "[example][minesweeper]") {
    auto f = makeFixture();
    f.script->onStart();

    // First child entity with a mesh = a cell.
    scene::EntityId someCell = 0;
    for (const auto& e : f.scene.entities()) {
        if (e.parent == f.rootId && e.mesh != nullptr) { someCell = e.id; break; }
    }
    REQUIRE(someCell != 0);

    f.script->onEntityClicked(someCell, 0);

    // Mines clicked are tinted bright red (1, 0, 0). Verify the clicked cell isn't.
    const auto* e = f.scene.findEntity(someCell);
    REQUIRE(e != nullptr);
    if (e->tint.has_value()) {
        const bool mineRed = e->tint->r > 0.95f && e->tint->g < 0.05f && e->tint->b < 0.05f;
        REQUIRE_FALSE(mineRed);
    }
}

TEST_CASE("Minesweeper4D right-click flags an unrevealed cell", "[example][minesweeper]") {
    auto f = makeFixture();
    f.script->onStart();

    scene::EntityId someCell = 0;
    for (const auto& e : f.scene.entities()) {
        if (e.parent == f.rootId && e.mesh != nullptr) { someCell = e.id; break; }
    }
    REQUIRE(someCell != 0);

    f.script->onEntityClicked(someCell, 1); // right-click = flag

    const auto* e = f.scene.findEntity(someCell);
    REQUIRE(e != nullptr);
    REQUIRE(e->tint.has_value());
    // Flagged tint is yellow (1, 0.85, 0).
    REQUIRE(e->tint->r > 0.95f);
    REQUIRE(e->tint->g > 0.80f);
    REQUIRE(e->tint->b < 0.05f);
}

TEST_CASE("Minesweeper4D::onMessage('restart') clears + respawns the grid", "[example][minesweeper]") {
    auto f = makeFixture();
    f.script->onStart();
    const size_t entitiesAfterStart = f.scene.entities().size();

    f.script->onMessage("restart");
    const size_t entitiesAfterRestart = f.scene.entities().size();

    // Same number of entities (root + 625 cells, plus status overlay made on the
    // first updateStatusText call -- restart re-creates none until onUpdate runs).
    REQUIRE(entitiesAfterStart >= 5 * 5 * 5 * 5 + 1); // root + cells
    REQUIRE(entitiesAfterRestart >= 5 * 5 * 5 * 5 + 1);
    // And there is still exactly 5^4 children of the root.
    int kids = 0;
    for (const auto& e : f.scene.entities()) {
        if (e.parent == f.rootId && e.mesh != nullptr) ++kids;
    }
    REQUIRE(kids == 5 * 5 * 5 * 5);
}
