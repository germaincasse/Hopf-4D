#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "geometry/Primitives.h"
#include "scene/Scene.h"
#include "scene/SceneSerializer.h"

#include <cstdio>
#include <filesystem>
#include <string>

using namespace hopf;
using Catch::Approx;

namespace {

std::string tempScenePath(const char* name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

} // namespace

TEST_CASE("Round-trip a tesseract + light + camera scene", "[serializer]") {
    const std::string path = tempScenePath("hopf_test_roundtrip.hopf");

    {
        scene::Scene s;
        s.setName("Demo");

        auto& tess = s.addPrimitive(geometry::PrimitiveType::Tesseract);
        tess.transform.position = {1.f, 2.f, 3.f, 4.f};
        tess.autoRotate.xy = 0.5f;
        tess.collider = scene::MeshColliderComponent{};
        tess.rigidbody = scene::RigidbodyComponent{};

        auto& sun = s.addEntity("Sun");
        sun.transform.position = {2.f, 2.f, 0.f, 0.f};
        scene::DirectionalLight l;
        l.intensity = 0.75f;
        l.r = 0.9f; l.g = 0.5f; l.b = 0.2f;
        sun.light = l;

        auto& cam = s.addEntity("Main");
        scene::Camera4DComponent c4;
        c4.focal4 = 5.5f; c4.isMain = true;
        cam.camera4D = c4;

        REQUIRE(scene::saveScene(s, path));
    }

    {
        scene::Scene s;
        REQUIRE(scene::loadScene(s, path));
        REQUIRE(s.entities().size() == 3);

        const auto& tess = s.entities()[0];
        REQUIRE(tess.primitiveType == "Tesseract");
        REQUIRE(tess.mesh != nullptr);
        REQUIRE(tess.transform.position.x == Approx(1.f));
        REQUIRE(tess.transform.position.w == Approx(4.f));
        REQUIRE(tess.autoRotate.xy        == Approx(0.5f));
        REQUIRE(tess.collider.has_value());
        REQUIRE(tess.rigidbody.has_value());

        const auto& sun = s.entities()[1];
        REQUIRE(sun.light.has_value());
        REQUIRE(sun.light->intensity == Approx(0.75f));
        REQUIRE(sun.light->r         == Approx(0.9f));
        REQUIRE(sun.light->b         == Approx(0.2f));
        REQUIRE(sun.transform.position.x == Approx(2.f));

        const auto& cam = s.entities()[2];
        REQUIRE(cam.camera4D.has_value());
        REQUIRE(cam.camera4D->focal4 == Approx(5.5f));
        REQUIRE(cam.camera4D->isMain);
    }

    std::remove(path.c_str());
}

TEST_CASE("Save/load preserves parent topology", "[serializer][parenting]") {
    const std::string path = tempScenePath("hopf_test_parent.hopf");

    scene::EntityId parentId = 0;
    scene::EntityId childId  = 0;
    {
        scene::Scene s;
        parentId = s.addEntity("Parent").id;
        childId  = s.addEntity("Child").id;
        s.setParent(childId, parentId);
        REQUIRE(scene::saveScene(s, path));
    }

    {
        scene::Scene s;
        REQUIRE(scene::loadScene(s, path));
        REQUIRE(s.entities().size() == 2);
        const auto& p = s.entities()[0];
        const auto& c = s.entities()[1];
        REQUIRE(p.id == parentId);
        REQUIRE(c.id == childId);
        REQUIRE(c.parent == parentId);
    }
    std::remove(path.c_str());
}

TEST_CASE("loadScene rejects an unknown header", "[serializer]") {
    const std::string path = tempScenePath("hopf_test_bad.hopf");
    {
        std::FILE* f = std::fopen(path.c_str(), "w");
        REQUIRE(f != nullptr);
        std::fputs("not-a-hopf-file 1\n", f);
        std::fclose(f);
    }
    scene::Scene s;
    REQUIRE_FALSE(scene::loadScene(s, path));
    std::remove(path.c_str());
}
