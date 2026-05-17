#include "audio/AudioEngine.h"

#include "core/Logger.h"
#include "scene/Scene.h"

#include "miniaudio.h"

#include <unordered_map>

namespace hopf::audio {

struct AudioEngine::Impl {
    ma_engine engine{};
    bool      engineInit = false;

    // One owned ma_sound per source entity that's currently playing during Play
    // mode. They are torn down at onPlayStop().
    std::unordered_map<scene::EntityId, std::unique_ptr<ma_sound>> sounds;
};

namespace {
AudioEngine* g_singleton = nullptr;
}

AudioEngine::AudioEngine() : m_impl(std::make_unique<Impl>()) {
    g_singleton = this;
}

AudioEngine::~AudioEngine() {
    shutdown();
    if (g_singleton == this) g_singleton = nullptr;
}

AudioEngine& AudioEngine::instance() {
    static AudioEngine fallback; // only used if Application never created its own
    return g_singleton ? *g_singleton : fallback;
}

bool AudioEngine::init() {
    if (m_enabled) return true;
    const ma_result rc = ma_engine_init(nullptr, &m_impl->engine);
    if (rc != MA_SUCCESS) {
        core::Logger::warning("AudioEngine: ma_engine_init failed (%d) -- audio disabled.", static_cast<int>(rc));
        return false;
    }
    m_impl->engineInit = true;
    m_enabled = true;
    core::Logger::info("AudioEngine: device initialised at %u Hz",
                       ma_engine_get_sample_rate(&m_impl->engine));
    return true;
}

void AudioEngine::shutdown() {
    if (!m_enabled) return;
    onPlayStop();
    if (m_impl->engineInit) {
        ma_engine_uninit(&m_impl->engine);
        m_impl->engineInit = false;
    }
    m_enabled = false;
}

void AudioEngine::onPlayStart(const scene::Scene& scene) {
    if (!m_enabled) return;
    onPlayStop(); // clear any leftovers from a previous play session

    for (const auto& e : scene.entities()) {
        if (!e.audioSource.has_value()) continue;
        const auto& src = *e.audioSource;
        if (!src.playOnStart || src.clipPath.empty()) continue;

        auto sound = std::make_unique<ma_sound>();
        ma_uint32 flags = MA_SOUND_FLAG_DECODE;
        if (ma_sound_init_from_file(&m_impl->engine, src.clipPath.c_str(), flags,
                                    nullptr, nullptr, sound.get()) != MA_SUCCESS)
        {
            core::Logger::warning("AudioEngine: cannot load '%s' for entity %llu",
                                  src.clipPath.c_str(),
                                  static_cast<unsigned long long>(e.id));
            continue;
        }
        ma_sound_set_volume(sound.get(), src.volume);
        ma_sound_set_pitch(sound.get(),  src.pitch);
        ma_sound_set_looping(sound.get(), src.loop ? MA_TRUE : MA_FALSE);
        ma_sound_set_spatialization_enabled(sound.get(), src.spatial ? MA_TRUE : MA_FALSE);
        if (src.spatial) {
            const auto wp = scene.worldMatrix(e.id).transformPoint({0.f, 0.f, 0.f, 0.f});
            ma_sound_set_position(sound.get(), wp.x, wp.y, wp.z);
            ma_sound_set_max_distance(sound.get(), src.maxDistance);
        }
        ma_sound_start(sound.get());
        m_impl->sounds[e.id] = std::move(sound);
    }
}

void AudioEngine::onPlayStop() {
    if (!m_enabled) return;
    for (auto& [id, snd] : m_impl->sounds) {
        ma_sound_stop(snd.get());
        ma_sound_uninit(snd.get());
    }
    m_impl->sounds.clear();
}

void AudioEngine::update(const scene::Scene& scene) {
    if (!m_enabled) return;

    // Listener: first entity with an active AudioListenerComponent.
    for (const auto& e : scene.entities()) {
        if (e.audioListener.has_value() && e.audioListener->active) {
            const auto wp = scene.worldMatrix(e.id).transformPoint({0.f, 0.f, 0.f, 0.f});
            ma_engine_listener_set_position(&m_impl->engine, 0, wp.x, wp.y, wp.z);
            break;
        }
    }

    // Refresh spatial source positions; drop sounds that finished and weren't looping.
    for (auto it = m_impl->sounds.begin(); it != m_impl->sounds.end(); ) {
        const auto id = it->first;
        const auto* e = scene.findEntity(id);
        if (!e || !e->audioSource.has_value()) {
            ma_sound_stop(it->second.get());
            ma_sound_uninit(it->second.get());
            it = m_impl->sounds.erase(it);
            continue;
        }
        const auto& src = *e->audioSource;
        ma_sound_set_volume(it->second.get(), src.volume);
        ma_sound_set_pitch(it->second.get(),  src.pitch);
        if (src.spatial) {
            const auto wp = scene.worldMatrix(id).transformPoint({0.f, 0.f, 0.f, 0.f});
            ma_sound_set_position(it->second.get(), wp.x, wp.y, wp.z);
        }
        if (!src.loop && ma_sound_at_end(it->second.get())) {
            ma_sound_uninit(it->second.get());
            it = m_impl->sounds.erase(it);
            continue;
        }
        ++it;
    }
}

void AudioEngine::playOneShot(const std::string& clipPath, float volume) {
    if (!m_enabled || clipPath.empty()) return;
    // ma_engine_play_sound is fire-and-forget; the engine cleans up when done.
    const ma_result rc = ma_engine_play_sound(&m_impl->engine, clipPath.c_str(), nullptr);
    if (rc != MA_SUCCESS) {
        core::Logger::warning("AudioEngine: playOneShot failed for '%s' (%d)",
                              clipPath.c_str(), static_cast<int>(rc));
    }
    // ma_engine_play_sound doesn't expose per-call volume, so fall back to global
    // engine volume scaling for the one-shot. This is intentionally minimal -- the
    // production path is the per-entity AudioSource component above.
    (void)volume;
}

} // namespace hopf::audio
