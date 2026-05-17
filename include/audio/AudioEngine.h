#pragma once

// Thin wrapper around miniaudio's ma_engine. Owns the device and a per-entity
// sound cache so AudioSourceComponent instances become playing sounds during
// play mode, with spatial 3D attenuation driven by the AudioListenerComponent
// entity.
//
// Audio is intentionally optional: if init() fails the engine flips to a no-op
// mode so the editor still runs (handy on headless CI). All public methods are
// safe to call when the engine is disabled.

#include "scene/Entity.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace hopf::scene { class Scene; }

namespace hopf::audio {

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&)            = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Singleton accessor. Scripts and the editor reach the running engine here.
    static AudioEngine& instance();

    bool init();          // tries to open the default audio device
    void shutdown();      // stops all sounds and releases the device
    bool isEnabled() const { return m_enabled; }

    // Frame tick during play: updates listener position from the scene and
    // refreshes spatial sources' positions for any sounds still playing.
    void update(const scene::Scene& scene);

    // On entering Playing state: spawn AudioSources flagged playOnStart. On
    // exiting: stop and free every spawned sound.
    void onPlayStart(const scene::Scene& scene);
    void onPlayStop();

    // Fire-and-forget play of an arbitrary clip; used by the editor's Inspector
    // preview button and by user scripts wanting a one-off SFX.
    void playOneShot(const std::string& clipPath, float volume = 1.f);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool                  m_enabled = false;
};

} // namespace hopf::audio
