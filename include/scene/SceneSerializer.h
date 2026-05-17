#pragma once

#include <string>

namespace hopf::scene {

class Scene;

// Plain-text scene file (.hopf). Round-trips entities, transforms and built-in
// components. Custom (non-primitive) meshes are not persisted -- the entity is saved
// without a mesh reference and the user would re-pair it on load.
// Returns false (with a logged error) on I/O or parse failure.
bool saveScene(const Scene& scene, const std::string& path);
bool loadScene(Scene& scene, const std::string& path);

} // namespace hopf::scene
