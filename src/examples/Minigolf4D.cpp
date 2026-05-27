// Example script: 4D mini-golf with 3 levels.
//
// The scene file is expected to already provide every gameplay entity:
//   - Ball, Ground, Hole
//   - WallN, WallS, WallE, WallW   (kinematic walls that keep the ball in)
//   - MinigolfStatus, MinigolfPower (UI overlays, updated by the script)
//   - a Main Camera (any entity with Camera4D.isMain)
//
// The script reconfigures Ground/Walls/Hole/Ball-spawn each time the player
// reaches a new level. Levels are defined in `kLevels` below.
//
// Controls (Play mode):
//   - Right-drag : orbit around the ball
//   - Wheel      : tilt the camera in the xw plane (peek along W)
//   - Left hold  : charge shot power; release to launch

#include "core/Logger.h"
#include "scene/Entity.h"
#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace {

constexpr float kWinRadius    = 0.9f;
constexpr float kPowerScale   = 8.f;
constexpr float kMaxCharge    = 2.0f;
constexpr float kCamDistance  = 6.f;
constexpr float kInitialPitch = -0.30f;

constexpr float kGroundY      = -2.f;
constexpr float kGroundHalfY  = 0.15f;          // world half-extent in Y after scale
constexpr float kBallSize     = 0.5f;           // world half-extent of the ball
constexpr float kBallY        = kGroundY + kGroundHalfY + kBallSize; // ball sits on ground

// Per-level layout. The script copies these into the scene entities at the
// start of each level. ground size is in WORLD half-extent units (i.e. the
// final value of (scale * primitive_half_extent) along X/Z).
struct LevelLayout {
    float groundHalfX;
    float groundHalfZ;
    hopf::math::Vec4 ballSpawn;
    hopf::math::Vec4 hole;
};

constexpr int kLevelCount = 3;
constexpr LevelLayout kLevels[kLevelCount] = {
    // Level 1: a compact 12x12 green, hole at the far corner.
    { 6.f, 6.f,
      {-4.f, kBallY, -4.f, 0.f},
      { 4.f, kGroundY + kGroundHalfY + 0.05f,  4.f, 0.f} },
    // Level 2: a long 20x6 fairway, hole at the far end.
    {10.f, 3.f,
      {-8.f, kBallY,  0.f, 0.f},
      { 8.f, kGroundY + kGroundHalfY + 0.05f,  0.f, 0.f} },
    // Level 3: a smaller 8x8 green, ball and hole on opposite corners.
    { 4.f, 4.f,
      {-3.f, kBallY,  3.f, 0.f},
      { 3.f, kGroundY + kGroundHalfY + 0.05f, -3.f, 0.f} },
};

constexpr float kWallHalfY      = 0.4f;        // wall world half-extent in Y
constexpr float kWallThickness  = 0.25f;       // wall world half-extent across the green
constexpr float kWallHalfW      = 4.f;         // walls stay extended in W like the ground

class Minigolf4D : public hopf::scripting::Script {
public:
    const char* typeName() const override { return "Minigolf4D"; }

    void onStart() override {
        bindEntities();
        m_yaw     = 0.f;
        m_pitch   = kInitialPitch;
        m_camXw   = 0.f;
        m_charge  = 0.f;
        m_holding = false;
        m_strokes = 0;
        m_won     = false;
        m_gameOver = false;
        m_currentLevel = 0;
        applyLevel(m_currentLevel);
        updateCamera();
        hopf::core::Logger::info("Minigolf4D: ready (3 levels). Right-drag orbit, wheel for 4D peek, hold left to charge.");
    }

    void onUpdate(float dt) override {
        if (m_holding && !m_won && !m_gameOver) {
            m_charge = std::min(kMaxCharge, m_charge + dt);
        }
        updateCamera();
        checkWin();
        updateUI();
    }

    void onMessage(const std::string& method) override {
        if (method == "restart") fullRestart();
    }

    void onMouseDragged(float dx, float dy, int button) override {
        if (button == 1) {
            m_yaw   += dx * 0.005f;
            m_pitch = std::clamp(m_pitch + dy * 0.005f, -1.40f, 1.40f);
        }
    }

    void onMouseWheel(float delta) override {
        m_camXw += delta * 0.10f;
    }

    void onMouseButton(int button, bool pressed) override {
        if (button != 0 || m_won || m_gameOver) return;
        if (pressed) {
            m_holding = true;
            m_charge  = 0.f;
        } else if (m_holding) {
            m_holding = false;
            launchBall();
        }
    }

private:
    hopf::scene::EntityId m_ballEntity   = 0;
    hopf::scene::EntityId m_groundEntity = 0;
    hopf::scene::EntityId m_holeEntity   = 0;
    hopf::scene::EntityId m_camEntity    = 0;
    hopf::scene::EntityId m_statusEntity = 0;
    hopf::scene::EntityId m_powerEntity  = 0;
    std::array<hopf::scene::EntityId, 4> m_walls{}; // N, S, E, W

    hopf::math::Vec4 m_spawnPos{};
    float m_yaw    = 0.f;
    float m_pitch  = kInitialPitch;
    float m_camXw  = 0.f;
    bool  m_holding  = false;
    float m_charge   = 0.f;
    int   m_strokes  = 0;
    int   m_currentLevel = 0;
    bool  m_won       = false;
    bool  m_gameOver  = false;
    float m_advanceTimer = 0.f; // small delay before auto-advancing to the next level

    void bindEntities() {
        m_ballEntity = m_groundEntity = m_holeEntity = 0;
        m_camEntity  = m_statusEntity = m_powerEntity = 0;
        for (auto& id : m_walls) id = 0;
        if (!scene()) return;
        for (auto& e : scene()->entities()) {
            if      (e.name == "Ball")            m_ballEntity   = e.id;
            else if (e.name == "Ground")          m_groundEntity = e.id;
            else if (e.name == "Hole")            m_holeEntity   = e.id;
            else if (e.name == "WallN")           m_walls[0]     = e.id;
            else if (e.name == "WallS")           m_walls[1]     = e.id;
            else if (e.name == "WallE")           m_walls[2]     = e.id;
            else if (e.name == "WallW")           m_walls[3]     = e.id;
            else if (e.name == "MinigolfStatus")  m_statusEntity = e.id;
            else if (e.name == "MinigolfPower")   m_powerEntity  = e.id;
            if (e.camera4D.has_value() && e.camera4D->isMain) m_camEntity = e.id;
        }
        if (m_ballEntity == 0)
            hopf::core::Logger::warning("Minigolf4D: no 'Ball' entity in scene.");
        if (m_camEntity == 0)
            hopf::core::Logger::warning("Minigolf4D: no main Camera 4D in scene.");
    }

    void fullRestart() {
        m_currentLevel = 0;
        m_strokes      = 0;
        m_won          = false;
        m_gameOver     = false;
        m_advanceTimer = 0.f;
        m_yaw    = 0.f;
        m_pitch  = kInitialPitch;
        m_camXw  = 0.f;
        m_charge = 0.f;
        m_holding = false;
        applyLevel(m_currentLevel);
        updateCamera();
    }

    void applyLevel(int idx) {
        if (idx < 0 || idx >= kLevelCount || !scene()) return;
        const auto& L = kLevels[idx];
        m_spawnPos = L.ballSpawn;

        // Ball: reset position + velocity.
        if (auto* b = scene()->findEntity(m_ballEntity)) {
            b->transform.position = L.ballSpawn;
            if (b->rigidbody.has_value()) {
                b->rigidbody->velocity        = {};
                b->rigidbody->angularVelocity = {};
            }
        }
        // Hole: reposition.
        if (auto* h = scene()->findEntity(m_holeEntity)) {
            h->transform.position = L.hole;
        }
        // Ground: rescale to (halfX, halfY_default, halfZ, kWallHalfW). The
        // tesseract primitive has half-extent 0.5, so scale = 2 * halfExtent.
        if (auto* g = scene()->findEntity(m_groundEntity)) {
            g->transform.position = {0.f, kGroundY, 0.f, 0.f};
            g->transform.scale    = {
                L.groundHalfX * 2.f,
                kGroundHalfY  * 2.f,
                L.groundHalfZ * 2.f,
                kWallHalfW    * 2.f,
            };
        }
        // Walls: position/orient each one at the edge of the green.
        positionWall(m_walls[0], { 0.f, kGroundY + kGroundHalfY + kWallHalfY,  L.groundHalfZ + kWallThickness, 0.f},
                                 { (L.groundHalfX + kWallThickness) * 2.f, kWallHalfY * 2.f, kWallThickness * 2.f, kWallHalfW * 2.f });
        positionWall(m_walls[1], { 0.f, kGroundY + kGroundHalfY + kWallHalfY, -L.groundHalfZ - kWallThickness, 0.f},
                                 { (L.groundHalfX + kWallThickness) * 2.f, kWallHalfY * 2.f, kWallThickness * 2.f, kWallHalfW * 2.f });
        positionWall(m_walls[2], {  L.groundHalfX + kWallThickness, kGroundY + kGroundHalfY + kWallHalfY, 0.f, 0.f},
                                 { kWallThickness * 2.f, kWallHalfY * 2.f, L.groundHalfZ * 2.f, kWallHalfW * 2.f });
        positionWall(m_walls[3], { -L.groundHalfX - kWallThickness, kGroundY + kGroundHalfY + kWallHalfY, 0.f, 0.f},
                                 { kWallThickness * 2.f, kWallHalfY * 2.f, L.groundHalfZ * 2.f, kWallHalfW * 2.f });
    }

    void positionWall(hopf::scene::EntityId id, const hopf::math::Vec4& pos,
                      const hopf::math::Vec4& scale)
    {
        if (auto* w = scene()->findEntity(id)) {
            w->transform.position = pos;
            w->transform.scale    = scale;
        }
    }

    void updateCamera() {
        if (!scene() || m_camEntity == 0 || m_ballEntity == 0) return;
        auto* cam  = scene()->findEntity(m_camEntity);
        auto* ball = scene()->findEntity(m_ballEntity);
        if (!cam || !ball) return;

        const float cyaw = std::cos(m_yaw),  syaw = std::sin(m_yaw);
        const float cp   = std::cos(m_pitch), sp = std::sin(m_pitch);
        const float fx =  syaw * cp;
        const float fy =  sp;
        const float fz = -cyaw * cp;

        const auto& bp = ball->transform.position;
        cam->transform.position = {
            bp.x - fx * kCamDistance,
            bp.y - fy * kCamDistance,
            bp.z - fz * kCamDistance,
            bp.w,
        };
        cam->transform.rotation    = hopf::math::Rotor4{};
        cam->transform.rotation.xz = m_yaw;
        cam->transform.rotation.yz = m_pitch;
        cam->transform.rotation.xw = m_camXw;
    }

    void launchBall() {
        if (!scene() || m_ballEntity == 0 || m_charge <= 0.01f) return;
        auto* ball = scene()->findEntity(m_ballEntity);
        if (!ball || !ball->rigidbody.has_value()) return;
        const float cyaw = std::cos(m_yaw), syaw = std::sin(m_yaw);
        const float speed = m_charge * kPowerScale;
        ball->rigidbody->velocity.x =  syaw * speed;
        ball->rigidbody->velocity.z = -cyaw * speed;
        ball->rigidbody->velocity.y = speed * 0.25f;
        ball->rigidbody->velocity.w = 0.f;
        ++m_strokes;
        m_charge = 0.f;
    }

    void checkWin() {
        if (!scene()) return;
        if (m_won) {
            // Small delay so the player sees the hole-in message, then advance.
            m_advanceTimer += 1.f / 60.f;
            if (m_advanceTimer > 1.2f) {
                if (m_currentLevel + 1 < kLevelCount) {
                    ++m_currentLevel;
                    m_won = false;
                    m_advanceTimer = 0.f;
                    applyLevel(m_currentLevel);
                } else {
                    m_gameOver = true;
                    m_won = false;
                }
            }
            return;
        }
        if (m_gameOver) return;
        auto* ball = scene()->findEntity(m_ballEntity);
        auto* hole = scene()->findEntity(m_holeEntity);
        if (!ball || !hole) return;
        const auto& a = ball->transform.position;
        const auto& b = hole->transform.position;
        const float dx = a.x - b.x, dz = a.z - b.z;
        if (std::sqrt(dx * dx + dz * dz) < kWinRadius && std::abs(a.y - b.y) < 1.5f) {
            m_won = true;
            m_advanceTimer = 0.f;
            if (ball->rigidbody.has_value()) ball->rigidbody->velocity = {};
            hopf::core::Logger::info("Minigolf4D: hole on level %d in %d stroke(s)!",
                                     m_currentLevel + 1, m_strokes);
        }
    }

    void updateUI() {
        if (!scene()) return;
        if (auto* s = scene()->findEntity(m_statusEntity); s && s->uiText.has_value()) {
            char buf[200];
            if (m_gameOver) {
                std::snprintf(buf, sizeof(buf), "*** GAME COMPLETE in %d strokes ***", m_strokes);
            } else if (m_won) {
                std::snprintf(buf, sizeof(buf), "Level %d clear! Next coming up...",
                              m_currentLevel + 1);
            } else {
                std::snprintf(buf, sizeof(buf),
                              "Level %d/%d   |   Strokes: %d   |   right-drag orbit, hold L to charge",
                              m_currentLevel + 1, kLevelCount, m_strokes);
            }
            s->uiText->text = buf;
        }
        if (auto* p = scene()->findEntity(m_powerEntity); p && p->uiRect.has_value()) {
            const float ratio = m_charge / kMaxCharge;
            p->uiRect->width = 300.f * std::max(0.02f, ratio);
            p->uiRect->r = std::min(1.f, 0.2f + ratio * 1.0f);
            p->uiRect->g = std::min(1.f, std::max(0.f, 0.9f - ratio));
            p->uiRect->b = 0.1f;
            p->visible   = m_holding && !m_won && !m_gameOver;
        }
    }
};

} // namespace

HOPF_REGISTER_SCRIPT(Minigolf4D)
