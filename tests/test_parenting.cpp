#include <catch2/catch_test_macros.hpp>

#include "scene/Scene.h"

using namespace hopf;

TEST_CASE("Scene::setParent links child to parent", "[scene][parenting]") {
    scene::Scene s;
    const auto aId = s.addEntity("A").id;
    const auto bId = s.addEntity("B").id;
    s.setParent(bId, aId);
    REQUIRE(s.findEntity(bId)->parent == aId);
}

TEST_CASE("Scene::setParent rejects cycles", "[scene][parenting]") {
    scene::Scene s;
    const auto aId = s.addEntity("A").id;
    const auto bId = s.addEntity("B").id;
    const auto cId = s.addEntity("C").id;

    s.setParent(bId, aId);   // b under a
    s.setParent(cId, bId);   // c under b

    // Try to parent a under c (would create a cycle).
    s.setParent(aId, cId);
    REQUIRE(s.findEntity(aId)->parent == 0);
}

TEST_CASE("Scene::worldMatrix composes parent translations", "[scene][parenting]") {
    scene::Scene s;
    const auto parentId = s.addEntity("Parent").id;
    s.findEntity(parentId)->transform.position = {10.f, 0.f, 0.f, 0.f};

    const auto childId = s.addEntity("Child").id;
    s.findEntity(childId)->transform.position = {0.f, 5.f, 0.f, 0.f};
    s.setParent(childId, parentId);

    const math::Vec4 world = s.worldMatrix(childId).transformPoint({0.f, 0.f, 0.f, 0.f});
    REQUIRE(world.x == 10.f);
    REQUIRE(world.y == 5.f);
    REQUIRE(world.z == 0.f);
    REQUIRE(world.w == 0.f);
}

TEST_CASE("Scene::worldMatrix returns identity for unknown id", "[scene][parenting]") {
    scene::Scene s;
    const math::Vec4 origin = s.worldMatrix(999).transformPoint({0.f, 0.f, 0.f, 0.f});
    REQUIRE(origin.x == 0.f);
    REQUIRE(origin.y == 0.f);
    REQUIRE(origin.z == 0.f);
    REQUIRE(origin.w == 0.f);
}
