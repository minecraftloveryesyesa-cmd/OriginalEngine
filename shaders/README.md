# SDL GPU shader sources

The GPU shader sources are kept in backend-specific formats.

## Vertex input

- Attribute 0: position, three 32-bit floats.
- Attribute 1: RGB color, three 32-bit floats.
- Vertex shader uniform slot 0: a 4x4 view-projection matrix.

## Metal (iOS)

`shaders/Metal` contains native Metal Shading Language source files for SDL_GPU's Metal backend. They are packaged into the iOS app bundle under `Resources/shaders`. The vertex shader expects the two vertex attributes above and a view-projection matrix at buffer slot 0.

SDL_GPU accepts MSL directly on Metal, so this path does not require SDL_shadercross at runtime. The files are source resources; they are not compiled into a metallib by this CMake configuration.

## HLSL

`shaders/Source` retains the HLSL pair for shadercross-based SPIR-V/DXIL workflows on other backends. The existing `shaders/compile.sh` script requires an installed `shadercross` executable.

## Integration status

This change adds the native MSL shader sources and bundles them with iOS builds. The runtime GPU preview still only clears the swapchain: shader loading, graphics pipeline creation, vertex/index buffer upload, depth testing, and scene drawing remain to be implemented.
