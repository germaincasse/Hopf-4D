#pragma once

namespace hopf::scene { class Scene; }

namespace hopf::physics {

// Advances the simulation by dt seconds. Applies autoRotate, gravity to rigidbodies,
// integrates velocity, and resolves AABB collisions between entities that share
// a collider dimension. Call once per frame while in Play mode.
void step(scene::Scene& scene, float dt);

} // namespace hopf::physics
