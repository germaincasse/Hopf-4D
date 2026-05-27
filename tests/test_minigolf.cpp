#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "geometry/Primitives.h"
#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

using namespace hopf;
using Catch::Approx;

namespace {

struct Fixture {
    scene::Scene            scene;
    scene::EntityId         rootId   = 0;
    scene::EntityId         ballId   = 0;
    scene::EntityId         groundId = 0;
    scene::EntityId         holeId   = 0;
    scene::EntityId         wallId[4]{0, 0, 0, 0};
    scene::EntityId         camId    = 0;
    scripting::Script*      script   = nullptr;
};

scene::EntityId addNamedPrimitive(scene::Scene& s, const char* name,
                                  geometry::PrimitiveType type,
                                  scene::EntityId parent,
                                  bool kinematic = false, bool gravity = false)
{
    auto& e = s.addPrimitive(type);
    e.name   = name;
    e.parent = parent;
    scene::MeshColliderComponent col; col.dimension = 4; e.collider = col;
    scene::RigidbodyComponent rb;
    rb.dimension  = 4;
    rb.kinematic  = kinematic;
    rb.useGravity = gravity;
    e.rigidbody = rb;
    return e.id;
}

Fixture makeFixture() {
    Fixture f;
    f.rootId = f.scene.addEntity("Minigolf").id;
    {
        auto* root = f.scene.findEntity(f.rootId);
        scene::ScriptInstance si;
        si.typeName = "Minigolf4D";
        si.instance = scripting::ScriptRegistry::instance().create("Minigolf4D");
        REQUIRE(si.instance != nullptr);
        root->scripts.push_back(std::move(si));
    }

    f.groundId   = addNamedPrimitive(f.scene, "Ground", geometry::PrimitiveType::Tesseract,        f.rootId, true,  false);
    f.ballId     = addNamedPrimitive(f.scene, "Ball",   geometry::PrimitiveType::Icositetrachoron, f.rootId, false, true);
    f.holeId     = f.scene.addPrimitive(geometry::PrimitiveType::Tesseract).id;
    f.scene.findEntity(f.holeId)->name = "Hole";
    f.scene.findEntity(f.holeId)->parent = f.rootId;
    f.wallId[0]  = addNamedPrimitive(f.scene, "WallN",  geometry::PrimitiveType::Tesseract, f.rootId, true, false);
    f.wallId[1]  = addNamedPrimitive(f.scene, "WallS",  geometry::PrimitiveType::Tesseract, f.rootId, true, false);
    f.wallId[2]  = addNamedPrimitive(f.scene, "WallE",  geometry::PrimitiveType::Tesseract, f.rootId, true, false);
    f.wallId[3]  = addNamedPrimitive(f.scene, "WallW",  geometry::PrimitiveType::Tesseract, f.rootId, true, false);

    auto& cam = f.scene.addEntity("Main Camera");
    cam.transform.position = {0, 2, 6, 0};
    cam.camera4D = scene::Camera4DComponent{};
    cam.camera4D->isMain = true;
    f.camId = cam.id;

    f.script = f.scene.findEntity(f.rootId)->scripts[0].instance.get();
    f.script->_bind(f.rootId, &f.scene);
    return f;
}

} // namespace

TEST_CASE("Minigolf4D::onStart spawns nothing new (declarative scene)", "[example][minigolf]") {
    auto f = makeFixture();
    const size_t before = f.scene.entities().size();
    f.script->onStart();
    REQUIRE(f.scene.entities().size() == before);
}

TEST_CASE("Minigolf4D::onStart applies level 1 layout to ball/ground/hole/walls", "[example][minigolf]") {
    auto f = makeFixture();
    f.script->onStart();
    // Ball spawn for level 1 is (-4, ~-1.3, -4, 0).
    const auto* ball = f.scene.findEntity(f.ballId);
    REQUIRE(ball->transform.position.x == Approx(-4.f));
    REQUIRE(ball->transform.position.z == Approx(-4.f));
    // Hole for level 1 is (4, ~-1.85, 4, 0).
    const auto* hole = f.scene.findEntity(f.holeId);
    REQUIRE(hole->transform.position.x == Approx(4.f));
    REQUIRE(hole->transform.position.z == Approx(4.f));
    // Ground X scale should match the level-1 half-extent (6) doubled.
    const auto* ground = f.scene.findEntity(f.groundId);
    REQUIRE(ground->transform.scale.x == Approx(12.f));
    REQUIRE(ground->transform.scale.z == Approx(12.f));
    // Each wall got positioned (non-default scale on at least X or Z).
    for (int i = 0; i < 4; ++i) {
        const auto* w = f.scene.findEntity(f.wallId[i]);
        REQUIRE(w != nullptr);
        REQUIRE((w->transform.scale.x > 0.4f && w->transform.scale.z > 0.4f));
    }
}

TEST_CASE("Minigolf4D ball uses Icositetrachoron for a smoother shape", "[example][minigolf]") {
    auto f = makeFixture();
    f.script->onStart();
    const auto* ball = f.scene.findEntity(f.ballId);
    REQUIRE(ball != nullptr);
    REQUIRE(ball->primitiveType == "Icositetrachoron");
}

TEST_CASE("Minigolf4D reaching the hole advances to the next level", "[example][minigolf]") {
    auto f = makeFixture();
    f.script->onStart();

    // Teleport the ball onto the hole.
    if (auto* ball = f.scene.findEntity(f.ballId)) {
        const auto* hole = f.scene.findEntity(f.holeId);
        ball->transform.position.x = hole->transform.position.x;
        ball->transform.position.z = hole->transform.position.z;
        ball->transform.position.y = hole->transform.position.y;
    }
    // First update: detect win on the current level.
    f.script->onUpdate(1.f / 60.f);
    // Simulate ~1.5s of post-win delay so the advance logic kicks in.
    for (int i = 0; i < 100; ++i) f.script->onUpdate(1.f / 60.f);

    // Ball should now be at level 2's spawn: (-8, ~-1.3, 0, 0).
    const auto* ball = f.scene.findEntity(f.ballId);
    REQUIRE(ball->transform.position.x == Approx(-8.f));
    REQUIRE(ball->transform.position.z == Approx(0.f));
}

TEST_CASE("Minigolf4D::onMessage('restart') sends the player back to level 1", "[example][minigolf]") {
    auto f = makeFixture();
    f.script->onStart();
    // Trigger a win → advance to level 2.
    if (auto* ball = f.scene.findEntity(f.ballId)) {
        const auto* hole = f.scene.findEntity(f.holeId);
        ball->transform.position.x = hole->transform.position.x;
        ball->transform.position.z = hole->transform.position.z;
        ball->transform.position.y = hole->transform.position.y;
    }
    for (int i = 0; i < 100; ++i) f.script->onUpdate(1.f / 60.f);
    REQUIRE(f.scene.findEntity(f.ballId)->transform.position.x == Approx(-8.f));

    // Restart → back to level 1 spawn (-4, *, -4, 0).
    f.script->onMessage("restart");
    const auto* ball = f.scene.findEntity(f.ballId);
    REQUIRE(ball->transform.position.x == Approx(-4.f));
    REQUIRE(ball->transform.position.z == Approx(-4.f));
}
