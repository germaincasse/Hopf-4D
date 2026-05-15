# Hopf

A 4D game engine written in C++ for Windows. Like Unity or Godot, but for games where the world has four spatial dimensions.

The name nods to the [Hopf fibration](https://en.wikipedia.org/wiki/Hopf_fibration), the canonical map from the 3-sphere to the 2-sphere. A small reminder that 4D space is rich, structured, and worth visiting.

![Hopf editor with a few 3D and 4D primitives in projection / depth-wireframe](docs/images/screenshot%201.png)

## Visualization

Two view modes per viewport:

- **Slice**: render the 3D intersection of the scene with a hyperplane perpendicular to one of the four axes. Scrub the offset to see the cross-section move.
- **Projection**: project the entire 4D scene to 3D (parallel or perspective along the chosen axis), then render.

Each viewport picks a display style independently: `Wire`, `Depth` (cool/warm gradient on the slice/projection axis), `Solid (unlit)`, `Solid (lit)`.

## Stack

- C++20, CMake 3.20+
- [GLFW](https://www.glfw.org/), [glad2](https://github.com/Dav1dde/glad), OpenGL 4.6 core
- [Dear ImGui](https://github.com/ocornut/imgui) (docking branch)
- [Catch2](https://github.com/catchorg/Catch2) v3 for tests

Windows-only for now.

## Repository layout

```
include/         public headers, organized by subsystem
src/             implementation files
tests/           Catch2 unit tests
external/cmake/  third-party fetch/build helpers
docs/            architecture notes
```

## Build

Requires Visual Studio 2022 (or Build Tools), CMake 3.20+, and Python 3 with `jinja2` (used by glad2's code generator: `pip install --user jinja2`).

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
build\bin\Release\Hopf.exe
```

Run the tests:

```powershell
ctest --test-dir build --output-on-failure -C Release
```

To skip building tests, configure with `-DHOPF_BUILD_TESTS=OFF`.

## Editor controls

- Right-click in the **Hierarchy** to add a primitive (2D, 3D, or 4D), a Light, or a Camera. Delete to remove, drag-drop to reorder.
- `Ctrl+D` duplicate, `Ctrl+C` / `Ctrl+V` copy/paste, `Del` delete the selected entity.
- `Ctrl+=` / `Ctrl+-` zoom the editor UI; `Ctrl+0` resets.
- In the **Viewport**: left-click selects what's under the cursor, drag pans. Right-click drag orbits, wheel zooms. `Camera...` opens all camera parameters in one popup; `Grid...` toggles per-plane grids.

## Roadmap

Hopf aims to grow into a fully usable 4D game engine. The roadmap below tracks what's done and what's planned.

### Done
- [x] Editor shell: dockable panels, dark Godot-style theme, multi-viewport
- [x] 2D, 3D, 4D primitives (Cube, Tesseract, 24-cell, Cubinder, Spherinder, duoprisms, etc.)
- [x] Slice and projection 4D->3D modes, four display styles, per-viewport camera
- [x] Camera 4D rotation (six rotation planes) + pivot orbit
- [x] Directional light (direction from entity rotation)
- [x] Per-plane grids with shared opacity
- [x] Click-to-select picking
- [x] Camera 2D / 3D / 4D components
- [x] Catch2 unit tests for math, geometry, scene

### Engine fundamentals
- [ ] Game runtime: play / pause / stop with scene snapshot + revert
- [ ] Game view panel rendering from a designated main camera
- [ ] Component system: arbitrary components attachable to entities
- [ ] Mesh / rigidbody / collider components in 2D, 3D, 4D
- [ ] 4D physics: collision detection between hypersolids, rigidbody dynamics
- [ ] Scripting layer (Lua or visual nodes)
- [ ] Project file format (.hopf) with serialization for scenes and assets
- [ ] Asset import pipeline (4D mesh formats, textures)
- [ ] Undo / redo stack across the editor

### Rendering
- [ ] More 4D primitives: 120-cell, 600-cell, glome (3-sphere), various duoprisms
- [ ] Custom 4D shaders / materials
- [ ] Per-cell colors for 4D meshes (visualize 4D structure better)
- [ ] Shadows (3D and 4D)
- [ ] Post-processing pipeline (bloom, tone-mapping, ambient occlusion)
- [ ] UI rendering (2D overlay system in viewport)

### Editor tooling
- [ ] Transform gizmos (translate / rotate / scale, with axis snapping)
- [ ] Multi-selection
- [ ] Prefab / nested entity hierarchies
- [ ] Inspector field locking, multi-edit
- [ ] Files panel: drag-import from filesystem
- [ ] Console: filtering, search, log levels

### Audio
- [ ] Audio source / listener components
- [ ] Spatial audio in 4D

### Build & deployment
- [ ] One-click "Build game" producing a standalone executable
- [ ] Asset packing
- [ ] Cross-platform (Linux, macOS) by swapping `platform/`

## License

[MIT](LICENSE).
