# OriginalEngine

A cross-platform game-engine project, starting with a C++20 foundation.

## Current status

- CMake-based C++20 project
- SDL3 window and event loop
- Animated perspective-projected 3D cube with six face colors
- Perspective grid to communicate depth
- Esc or window close exits the application
- GitHub Actions builds the Windows executable and uploads it as an artifact

> The current renderer is an initial CPU-side projection and SDL triangle-rasterization prototype, not yet a modern GPU 3D pipeline. Depth sorting uses a painter's algorithm, so intersecting geometry and complex scenes will need a depth-buffer renderer later.

## Requirements

- CMake 3.24+
- A C++20 compiler
- Git and internet access during the first configure (SDL3 is downloaded by CMake)

## Build on Windows

Run in a terminal with CMake and MSVC available:

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
.\build\Release\originalengine_app.exe
```

## Build through GitHub Actions (no local Windows PC required)

1. Open the repository's Actions tab.
2. Select the Build workflow and choose Run workflow, or push a commit to main.
3. Open the completed run and download the OriginalEngine-Windows-x64 artifact.
4. Extract the ZIP and run originalengine_app.exe on a compatible Windows PC.

GitHub Actions can compile the Windows app, but it cannot show you the interactive window inside the build log. You need a Windows device to run and visually inspect the executable.

## Project layout

```text
include/engine/       Public engine headers
src/engine/           Engine implementations
src/main.cpp          Sample application entry point
.github/workflows/    GitHub Actions workflow
```

## Roadmap

1. Initial application and build foundation
2. First 3D projection and colored cube
3. Camera controls and mesh abstraction
4. GPU-backed rendering and depth testing
5. Texture and model loading
6. Scene management, audio, and physics
7. Platform support expansion
