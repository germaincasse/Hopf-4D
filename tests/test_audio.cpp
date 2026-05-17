#include <catch2/catch_test_macros.hpp>

#include "audio/AudioEngine.h"
#include "scene/Scene.h"

using namespace hopf;

// These tests exercise the engine's lifecycle without asserting that a real
// audio device exists -- CI may not have one. The engine flips to a no-op mode
// in that case; we just check it stays well-behaved under repeated calls.

TEST_CASE("AudioEngine init/shutdown is idempotent", "[audio]") {
    audio::AudioEngine eng;
    const bool a = eng.init();
    (void)a;
    eng.init();      // re-init no-ops
    eng.shutdown();
    eng.shutdown();  // re-shutdown no-ops
    REQUIRE(eng.isEnabled() == false);
}

TEST_CASE("AudioEngine onPlayStart/onPlayStop work on an empty scene", "[audio]") {
    scene::Scene s;
    audio::AudioEngine eng;
    eng.init();
    eng.onPlayStart(s);
    eng.update(s);
    eng.onPlayStop();
    eng.shutdown();
}

TEST_CASE("AudioEngine handles an entity with an unloadable clip gracefully", "[audio]") {
    scene::Scene s;
    auto& e = s.addEntity("Snd");
    scene::AudioSourceComponent src;
    src.clipPath    = "this/file/does/not/exist.wav";
    src.playOnStart = true;
    e.audioSource   = src;

    audio::AudioEngine eng;
    eng.init();
    eng.onPlayStart(s);   // should warn and skip, not throw
    eng.update(s);
    eng.onPlayStop();
    eng.shutdown();
}

TEST_CASE("AudioEngine::instance returns a usable engine", "[audio]") {
    auto& e = audio::AudioEngine::instance();
    // No exceptions; init may or may not succeed depending on the environment.
    e.init();
    e.shutdown();
}
