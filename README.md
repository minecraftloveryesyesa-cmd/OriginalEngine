# OriginalEngine

A cross-platform game-engine project, starting with a C++20 foundation.

## Current status

- CMake-based C++20 project
- SDL3 window and event loop
- First-person camera with WASD movement and mouse-look
- Axis-aligned collision world; camera movement is blocked by obstacles and slides along walls
- Basic scene registry with unique object names, transform data, visibility flags, lookup, and removal
- Wavefront OBJ geometry loader for vertices and faces, including polygon triangulation and negative indices
- Q/E vertical movement; Shift for faster movement
- Right-click toggles mouse capture; Esc releases the mouse or exits
- Animated perspective-projected 3D cube with six face colors and all three rotation axes
- Loads and draws the bundled `assets/demo.obj` pyramid through the scene transform
- World-space perspective floor grid that responds to camera movement
- GitHub Actions builds the Windows executable, runs camera, collision, scene-management, and OBJ-loader unit tests plus a rendering smoke test, and uploads the executable as an artifact

> The current renderer is an initial CPU-side projection and SDL triangle-rasterization prototype, not yet a modern GPU 3D pipeline. Depth sorting uses a painter's algorithm; intersecting geometry and complex scenes will need a depth-buffer renderer later.

## Camera controls

- **W / S:** move forward / backward
- **A / D:** strafe left / right
- **Q / E:** move down / up
- **Left or right Shift:** move faster
- **Right mouse button:** capture/release the mouse for looking around
- **Mouse movement:** look around while captured
- **Esc:** release the mouse; press again to exit

## OBJ mesh loader prototype

`ObjMeshLoader` reads Wavefront OBJ geometry (`v` and `f` records), triangulates polygon faces, supports positive and negative vertex indices, and ignores texture/normal/material records for now. It validates face indices and leaves the caller's existing mesh unchanged on failure. The bundled `assets/demo.obj` pyramid is loaded into the demo scene and rendered with its object transform. Texture coordinates, normals, material files, clipping for triangles crossing the near plane, and texture/material rendering are not implemented yet.

## Collision prototype

The collision module supports static axis-aligned bounding boxes (AABBs). Camera movement resolves X, Y and Z separately to prevent entering a box and to allow sliding along its faces. This is a prototype; rotating meshes, slopes, stair stepping and a full capsule controller are not implemented yet.

## Requirements

- CMake 3.24+
- A C++20 compiler
- Git and internet access during the first configure (SDL3 is downloaded by CMake)

## Build on Windows

Run in a terminal with CMake and MSVC available:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\originalengine_app.exe
```

## Build through GitHub Actions (no local Windows PC required)

1. Open the repository's Actions tab.
2. Select the Build workflow and choose Run workflow, or push a commit to main.
3. Open the completed run and download the OriginalEngine-Windows-x64 artifact.
4. Extract the ZIP and run originalengine_app.exe on a compatible Windows PC.

GitHub Actions can compile the Windows app and run automated tests, but it cannot show the interactive window inside the build log. You need a Windows device to run and visually inspect the executable.

## Project layout

```text
include/engine/       Public engine headers
src/engine/           Engine implementations
src/main.cpp          Sample application entry point
tests/                Automated camera, collision, and scene tests
.github/workflows/    GitHub Actions workflow
```

## Roadmap

1. Initial application and build foundation
2. First 3D projection and colored cube
3. First-person camera and movement
4. GPU-backed rendering and depth testing
5. Render scene objects and loaded meshes
6. Texture/material loading, audio, and physics
7. Platform support expansion
