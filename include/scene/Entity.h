#pragma once

#include "geometry/Mesh4D.h"
#include "scene/Transform4D.h"

#include <cstdint>
#include <string>

namespace hopf::scene {

using EntityId = uint64_t;

// Per-plane angular velocities (rad/s). Identity = static.
struct AutoRotate {
    float xy = 0.f;
    float xz = 0.f;
    float xw = 0.f;
    float yz = 0.f;
    float yw = 0.f;
    float zw = 0.f;
};

struct Entity {
    EntityId                id = 0;
    std::string             name;
    Transform4D             transform;
    const geometry::Mesh4D* mesh = nullptr; // owned by Scene
    bool                    visible    = true;
    AutoRotate              autoRotate{};
};

} // namespace hopf::scene
