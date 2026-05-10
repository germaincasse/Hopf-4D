# Architecture

High-level design notes. The codebase is organized in layers; each layer only knows about layers strictly below it.

```
editor/                 dockable UI (Dear ImGui)
  -> render scene geometry math platform core

render/                 4D->3D pipeline (Slicer, Projector), GL plumbing
  -> scene geometry math platform

scene/                  Scene graph, Entity, Transform4D
  -> geometry math

geometry/               Mesh4D, primitive builders
  -> math

math/                   Vec4, Mat4, Mat5, Rotor4 (header-only)

platform/               Window/input wrappers (GLFW)

core/                   Application loop, Logger
  -> editor platform
```

## File layout

```
include/<sub>/   public headers
src/<sub>/       implementations
tests/           Catch2 unit tests
external/cmake/  Dependencies.cmake (FetchContent fetches GLFW, glad2, ImGui, Catch2)
```

## 4D math

- **`Vec4`**: point or direction in `(x, y, z, w)`.
- **`Mat5`**: 5x5 column-major matrix for affine 4D transforms (the 5th coordinate is the homogeneous one). Translation, scale, and the six plane rotations all compose into a single matrix.
- **`Rotor4`**: six per-plane rotation angles (`xy`, `xz`, `xw`, `yz`, `yw`, `zw`). 4D has six independent rotation planes, so there is no single "axis of rotation" the way there is in 3D. The representation isn't free of gimbal-lock-like artifacts (rotations don't commute) but it's editor-friendly. A bivector-rotor implementation can replace it later without touching callers.
- **`Mat4`**: column-major 4x4, used for the 3D view/projection stage that follows the 4D->3D step.

## Geometry

`Mesh4D` is the canonical container:

- `vertices`   : `Vec4`
- `edges`      : pairs of vertex indices (wireframe)
- `triangles`  : surface triangles (solid styles)
- `tetrahedra` : 3-cells, used by the Slicer; non-tetrahedral cells must be triangulated at build time

Primitives ship in `Primitives.cpp`: cube, tetrahedron, octahedron, UV sphere, tesseract (8-cell), pentachoron (5-cell), hexadecachoron (16-cell). 3D primitives live at `w = 0` (no tetrahedra, hidden in slice mode unless `sliceW = 0`).

## Rendering

Each viewport runs its own `ViewportRenderer` that draws into an FBO; the editor displays the resulting color texture inside an ImGui Image.

The 4D->3D step picks one of two strategies per frame:

### Slicer

For each tetrahedral cell, intersects with the hyperplane `w = sliceW` (in camera space) using marching-tetrahedra. Each cell yields nothing, a triangle, or a quadrilateral (split into two triangles). The output is a 3D triangle soup ready for standard rasterization.

### Projector

Drops `w` (parallel) or scales `xyz` by `focal4 / (w + wOffset)` (perspective) to produce 3D positions. Output carries both edges (for wireframe styles) and triangle indices (for solid styles).

### Camera 4D

`Camera4D` bundles all viewport state: 4D pose (`position4`, `rotation4`), view mode and display style, mode-specific knobs, and the 3D orbit camera (pivot + yaw/pitch/distance). `rotation4` is applied as the inverse view transform before slicing/projecting, so non-trivial rotation in any plane involving `w` reveals the W axis.

## Editor

Dear ImGui dockspace with panels:

- **Viewport**: owns a `Camera4D` and a `ViewportRenderer`.
- **Hierarchy**: entity tree with right-click add menu and drag-drop reorder.
- **Inspector**: properties of the selected entity (transform, auto-rotate, mesh stats).
- **Console**: log sink reading from `core::Logger`.
- **Files**: read-only project tree.

Panels are stateless w.r.t. the scene; they read/write through `EditorContext` which the `Editor` owns and threads through each render call. Selection, clipboard, and the active scene pointer all live there.

## Why OpenGL

OpenGL 4.6 core is the lowest-friction backend on Windows that still gives us shader-based pipelines. The renderer is small and isolated, so switching to D3D11 or Vulkan later is a `render/` rewrite, not an engine rewrite.
