#include <catch2/catch_test_macros.hpp>

#include "editor/EditorContext.h"
#include "scene/Scene.h"

using namespace hopf;

TEST_CASE("EditorContext::undo restores a previous entities snapshot", "[editor][undo]") {
    scene::Scene s;
    editor::EditorContext ctx;
    ctx.scene = &s;

    s.addEntity("A");
    ctx.pushUndo();
    s.addEntity("B");
    REQUIRE(s.entities().size() == 2);

    ctx.undo();
    REQUIRE(s.entities().size() == 1);
    REQUIRE(s.entities()[0].name == "A");
}

TEST_CASE("EditorContext::undo is no-op when stack is empty", "[editor][undo]") {
    scene::Scene s;
    editor::EditorContext ctx;
    ctx.scene = &s;
    s.addEntity("X");

    ctx.undo();
    REQUIRE(s.entities().size() == 1);
}

TEST_CASE("EditorContext::undo clears stale selection", "[editor][undo]") {
    scene::Scene s;
    editor::EditorContext ctx;
    ctx.scene = &s;

    ctx.pushUndo();                     // empty snapshot
    auto& e = s.addEntity("Picked");
    ctx.selected = e.id;
    REQUIRE(s.entities().size() == 1);

    ctx.undo();
    REQUIRE(s.entities().empty());
    REQUIRE(ctx.selected == 0);
}

TEST_CASE("EditorContext::undo respects the 50-snapshot cap", "[editor][undo]") {
    scene::Scene s;
    editor::EditorContext ctx;
    ctx.scene = &s;

    for (int i = 0; i < 200; ++i) ctx.pushUndo();
    REQUIRE(ctx.undoStack.size() == 50);
}
