#include <catch2/catch_test_macros.hpp>

#include "hopf/geometry/Primitives.h"
#include "hopf/scene/Scene.h"

using namespace hopf;

TEST_CASE("Scene assigns unique entity ids", "[scene]") {
    scene::Scene s;
    auto& a = s.addEntity("A");
    auto& b = s.addEntity("B");
    REQUIRE(a.id != b.id);
}

TEST_CASE("Scene::addPrimitive binds mesh and entity", "[scene]") {
    scene::Scene s;
    auto& e = s.addPrimitive(geometry::PrimitiveType::Tesseract);
    REQUIRE(e.mesh != nullptr);
    REQUIRE(e.mesh->vertices.size() == 16);
    REQUIRE(e.name == std::string("Tesseract"));
}

TEST_CASE("Scene::removeEntity clears the slot", "[scene]") {
    scene::Scene s;
    const auto aid = s.addEntity("A").id;
    const auto bid = s.addEntity("B").id;

    REQUIRE(s.removeEntity(aid));
    REQUIRE(s.entities().size() == 1);
    REQUIRE(s.findEntity(bid) != nullptr);
    REQUIRE(!s.removeEntity(9999));
}

TEST_CASE("Scene::moveEntityBefore reorders", "[scene]") {
    scene::Scene s;
    const auto aid = s.addEntity("A").id;
    s.addEntity("B");
    const auto cid = s.addEntity("C").id;

    s.moveEntityBefore(cid, aid);
    REQUIRE(s.entities()[0].name == "C");
    REQUIRE(s.entities()[1].name == "A");
    REQUIRE(s.entities()[2].name == "B");
}
