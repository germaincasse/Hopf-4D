// Example script: 4D Minesweeper on a 5x5x5x5 tesseract grid.
//
// Attach this script to a root entity. On Play it spawns 625 cell entities as
// children. The first click generates mines (avoiding the clicked cell and its
// neighbors). Left-click reveals; right-click flags. Neighbors are the 3^4-1=80
// king-move neighbors. Cells are colored by neighbor count, hidden when empty.
//
// Add a UI button with onClickScript="Minesweeper4D", onClickMethod="restart"
// to wire a New Game button.

#include "core/Logger.h"
#include "geometry/Primitives.h"
#include "scene/Scene.h"
#include "scripting/Script.h"
#include "scripting/ScriptRegistry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

constexpr int   kGridSize  = 5;
constexpr int   kCellCount = kGridSize * kGridSize * kGridSize * kGridSize;
constexpr float kSpacing   = 2.5f;
constexpr int   kMineCount = 60;

inline int indexOf(int x, int y, int z, int w) {
    return ((x * kGridSize + y) * kGridSize + z) * kGridSize + w;
}
inline void coordsOf(int idx, int& x, int& y, int& z, int& w) {
    w = idx % kGridSize; idx /= kGridSize;
    z = idx % kGridSize; idx /= kGridSize;
    y = idx % kGridSize; idx /= kGridSize;
    x = idx;
}
inline bool inBounds(int x, int y, int z, int w) {
    return x >= 0 && x < kGridSize && y >= 0 && y < kGridSize
        && z >= 0 && z < kGridSize && w >= 0 && w < kGridSize;
}

class Minesweeper4D : public hopf::scripting::Script {
public:
    const char* typeName() const override { return "Minesweeper4D"; }

    void onStart() override {
        m_rng.seed(std::random_device{}());
        spawnGrid();
        hopf::core::Logger::info("Minesweeper4D: 5^4 grid spawned, %d mines hidden after first click.",
                                 kMineCount);
    }

    void onEntityClicked(hopf::scene::EntityId clicked, int button) override {
        auto it = m_cellByEntity.find(clicked);
        if (it == m_cellByEntity.end()) return;
        const int idx = it->second;
        if (m_status == Status::Won || m_status == Status::Lost) return;

        if (button == 0) {
            if (m_cells[idx].flagged) return;
            if (m_status == Status::NotStarted) {
                generateMines(idx);
                m_status = Status::Playing;
            }
            revealCell(idx);
        } else if (button == 1) {
            toggleFlag(idx);
        }
    }

    void onMouseWheel(float delta) override {
        auto* cam = mainCamera();
        if (!cam) return;
        const float current = cam->camera4D->sliceVal;
        const float target  = std::round(current / kSpacing) * kSpacing
                            + (delta > 0 ? kSpacing : -kSpacing);
        const float bound   = kSpacing * (kGridSize - 1) * 0.5f;
        cam->camera4D->sliceVal = std::clamp(target, -bound, bound);
    }

    void onMessage(const std::string& method) override {
        if (method == "restart") {
            despawnGrid();
            spawnGrid();
            hopf::core::Logger::info("Minesweeper4D: restarted.");
        }
    }

    void onUpdate(float /*dt*/) override {
        updateStatusText();
    }

private:
    enum class Status { NotStarted, Playing, Won, Lost };

    struct Cell {
        hopf::scene::EntityId entity = 0;
        int  count    = 0;
        bool isMine   = false;
        bool revealed = false;
        bool flagged  = false;
    };

    std::array<Cell, kCellCount> m_cells{};
    std::unordered_map<hopf::scene::EntityId, int> m_cellByEntity;
    int    m_revealedCount = 0;
    int    m_flaggedCount  = 0;
    Status m_status        = Status::NotStarted;
    std::mt19937 m_rng;
    hopf::scene::EntityId m_statusEntity = 0;

    hopf::scene::Entity* mainCamera() {
        if (!scene()) return nullptr;
        for (auto& e : scene()->entities()) {
            if (e.camera4D.has_value() && e.camera4D->isMain) return &e;
        }
        return nullptr;
    }

    void spawnGrid() {
        if (!scene()) return;
        for (auto& c : m_cells) c = Cell{};
        m_cellByEntity.clear();
        m_revealedCount = 0;
        m_flaggedCount  = 0;
        m_status        = Status::NotStarted;
        m_statusEntity  = 0;

        const auto rootId = entityId();
        const float c = (kGridSize - 1) * 0.5f;

        // Build one tesseract mesh and share it across all 625 cells.
        const auto* sharedMesh = scene()->takeOwnedMesh(
            hopf::geometry::buildPrimitive(hopf::geometry::PrimitiveType::Tesseract, 0.4f));

        for (int x = 0; x < kGridSize; ++x)
        for (int y = 0; y < kGridSize; ++y)
        for (int z = 0; z < kGridSize; ++z)
        for (int w = 0; w < kGridSize; ++w) {
            const int idx = indexOf(x, y, z, w);
            auto& cell = scene()->addEntity("Cell");
            cell.parent = rootId;
            cell.mesh   = sharedMesh;
            cell.primitiveType = "Tesseract";
            cell.transform.position = {
                (x - c) * kSpacing,
                (y - c) * kSpacing,
                (z - c) * kSpacing,
                (w - c) * kSpacing,
            };
            cell.tint = hopf::scene::ColorTint{0.55f, 0.55f, 0.6f}; // unrevealed gray
            m_cells[idx].entity = cell.id;
            m_cellByEntity[cell.id] = idx;
        }
    }

    void despawnGrid() {
        if (!scene()) return;
        for (const auto& c : m_cells) {
            if (c.entity != 0) scene()->removeEntity(c.entity);
        }
        if (m_statusEntity != 0) scene()->removeEntity(m_statusEntity);
        m_cellByEntity.clear();
    }

    void generateMines(int avoidIdx) {
        // Safe-start: avoidIdx and all 80 of its king-move neighbours stay mine-free.
        int ax, ay, az, aw; coordsOf(avoidIdx, ax, ay, az, aw);
        std::vector<bool> safe(kCellCount, false);
        for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
        for (int dw = -1; dw <= 1; ++dw) {
            const int nx = ax + dx, ny = ay + dy, nz = az + dz, nw = aw + dw;
            if (!inBounds(nx, ny, nz, nw)) continue;
            safe[indexOf(nx, ny, nz, nw)] = true;
        }

        std::vector<int> candidates;
        candidates.reserve(kCellCount);
        for (int i = 0; i < kCellCount; ++i) if (!safe[i]) candidates.push_back(i);
        std::shuffle(candidates.begin(), candidates.end(), m_rng);

        const int mines = std::min<int>(kMineCount, static_cast<int>(candidates.size()));
        for (int i = 0; i < mines; ++i) m_cells[candidates[i]].isMine = true;

        for (int i = 0; i < kCellCount; ++i) {
            if (!m_cells[i].isMine) m_cells[i].count = countNeighbourMines(i);
        }
    }

    int countNeighbourMines(int idx) const {
        int x, y, z, w; coordsOf(idx, x, y, z, w);
        int count = 0;
        for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy)
        for (int dz = -1; dz <= 1; ++dz)
        for (int dw = -1; dw <= 1; ++dw) {
            if (dx == 0 && dy == 0 && dz == 0 && dw == 0) continue;
            const int nx = x + dx, ny = y + dy, nz = z + dz, nw = w + dw;
            if (!inBounds(nx, ny, nz, nw)) continue;
            if (m_cells[indexOf(nx, ny, nz, nw)].isMine) ++count;
        }
        return count;
    }

    void revealCell(int idx) {
        if (m_cells[idx].revealed || m_cells[idx].flagged) return;
        if (m_cells[idx].isMine) {
            m_status = Status::Lost;
            for (const auto& c : m_cells) {
                if (c.isMine) tintEntity(c.entity, 0.9f, 0.2f, 0.2f);
            }
            tintEntity(m_cells[idx].entity, 1.0f, 0.0f, 0.0f);
            return;
        }
        cascade(idx);
        if (m_revealedCount + kMineCount >= kCellCount) {
            m_status = Status::Won;
            for (const auto& c : m_cells) {
                if (c.isMine) tintEntity(c.entity, 0.2f, 1.0f, 0.4f);
            }
        }
    }

    void cascade(int start) {
        std::vector<int> stack{start};
        while (!stack.empty()) {
            const int idx = stack.back();
            stack.pop_back();
            if (m_cells[idx].revealed || m_cells[idx].flagged || m_cells[idx].isMine) continue;
            m_cells[idx].revealed = true;
            ++m_revealedCount;
            applyRevealedTint(idx);
            if (m_cells[idx].count == 0) {
                int x, y, z, w; coordsOf(idx, x, y, z, w);
                for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                for (int dz = -1; dz <= 1; ++dz)
                for (int dw = -1; dw <= 1; ++dw) {
                    if (dx == 0 && dy == 0 && dz == 0 && dw == 0) continue;
                    const int nx = x + dx, ny = y + dy, nz = z + dz, nw = w + dw;
                    if (!inBounds(nx, ny, nz, nw)) continue;
                    stack.push_back(indexOf(nx, ny, nz, nw));
                }
            }
        }
    }

    void applyRevealedTint(int idx) {
        const auto& c = m_cells[idx];
        if (c.count == 0) {
            auto* e = scene()->findEntity(c.entity);
            if (e) e->visible = false;
            return;
        }
        // Color gradient over typical counts (1..20 in this 4D variant): blue->yellow->red.
        const float t = std::min(1.f, c.count / 16.f);
        const float r = std::min(1.f, t * 1.6f);
        const float g = std::min(1.f, std::max(0.f, 1.6f - t * 1.6f));
        const float b = std::min(1.f, std::max(0.f, 1.f - t * 2.f));
        tintEntity(c.entity, r, g, b);
    }

    void toggleFlag(int idx) {
        if (m_cells[idx].revealed) return;
        m_cells[idx].flagged = !m_cells[idx].flagged;
        m_flaggedCount += m_cells[idx].flagged ? 1 : -1;
        tintEntity(m_cells[idx].entity,
                   m_cells[idx].flagged ? 1.0f : 0.55f,
                   m_cells[idx].flagged ? 0.85f : 0.55f,
                   m_cells[idx].flagged ? 0.0f : 0.6f);
    }

    void tintEntity(hopf::scene::EntityId id, float r, float g, float b) {
        if (!scene()) return;
        auto* e = scene()->findEntity(id);
        if (e) e->tint = hopf::scene::ColorTint{r, g, b};
    }

    void updateStatusText() {
        if (!scene()) return;
        if (m_statusEntity == 0) {
            auto& e = scene()->addEntity("MinesweeperStatus");
            e.parent = entityId();
            hopf::scene::UIRectComponent rect;
            rect.anchor = hopf::scene::UIAnchor::TopLeft;
            rect.x = 220.f; rect.y = 30.f;
            rect.width = 440.f; rect.height = 36.f;
            rect.r = 0.05f; rect.g = 0.05f; rect.b = 0.08f; rect.a = 0.7f;
            e.uiRect = rect;
            hopf::scene::UITextComponent t;
            t.text = "Click any tesseract to start.";
            t.fontSize = 18.f;
            e.uiText = t;
            m_statusEntity = e.id;
        }
        auto* e = scene()->findEntity(m_statusEntity);
        if (!e || !e->uiText.has_value()) return;
        const char* st = "Playing";
        switch (m_status) {
            case Status::NotStarted: st = "Click any tesseract to start"; break;
            case Status::Playing:    st = "Playing";                        break;
            case Status::Won:        st = "*** YOU WIN ***";                break;
            case Status::Lost:       st = "*** GAME OVER ***";              break;
        }
        char buf[256];
        std::snprintf(buf, sizeof(buf),
                      "%s   |   flags %d/%d   |   revealed %d/%d   |   wheel = next slice",
                      st, m_flaggedCount, kMineCount,
                      m_revealedCount, kCellCount - kMineCount);
        e->uiText->text = buf;
    }
};

} // namespace

HOPF_REGISTER_SCRIPT(Minesweeper4D)
