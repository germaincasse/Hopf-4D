# Hopf

A 4D game engine written in C++ for Windows. Like Unity or Godot, but for games where the world has four spatial dimensions.

The name nods to the [Hopf fibration](https://en.wikipedia.org/wiki/Hopf_fibration), the canonical map from the 3-sphere to the 2-sphere — a small reminder that 4D space is rich, structured, and worth visiting.

![Hopf editor with a few 3D and 4D primitives in projection / depth-wireframe](docs/images/screenshot%201.png)

## Visualization

Two view modes per viewport:

- **Slice** — render the 3D intersection of the scene with a hyperplane `w = w0`. Scrub the plane to see the cross-section move.
- **Projection** — project the entire 4D scene to 3D (parallel or perspective along `w`), then render.

Each viewport picks a display style independently: `Wire`, `Depth` (cool/warm gradient on `w`), `Solid (unlit)`, `Solid (lit)`.

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

- Right-click in the **Hierarchy** to add a primitive (3D or 4D), Delete to remove, drag-drop to reorder.
- `Ctrl+D` duplicate, `Ctrl+C` / `Ctrl+V` copy/paste, `Del` delete the selected entity.
- In the **Viewport**: left-click drag pans, right-click drag orbits (no pitch clamp), wheel zooms. The orbit pivot follows pans.

## Roadmap

- [x] Editor with dockable panels
- [x] Tesseract / 5-cell / 16-cell + 3D primitives
- [x] Slice and projection modes, four display styles
- [x] Pivot orbit camera + 4D camera rotation
- [x] Catch2 unit tests for math, geometry, scene
- [ ] More 4D primitives (120-cell, 600-cell, hypersphere)
- [ ] Project file format + asset import
- [ ] Scripting layer
- [ ] 4D physics

## License

[MIT](LICENSE).
