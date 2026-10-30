# SDL GPU shader sources

This directory contains the shared HLSL shader pair intended for the SDL GPU backend.

## Vertex input

- Attribute 0: position, three 32-bit floats.
- Attribute 1: RGB color, three 32-bit floats.
- Vertex shader uniform slot 0: a 4x4 view-projection matrix.

The vertex shader transforms each position with the supplied view-projection matrix. The fragment shader clamps interpolated RGB values to the valid 0–1 range and writes an opaque color.

## Compile

Install SDL_shadercross and make sure `shadercross` is on PATH.

- macOS / Linux: run `sh shaders/compile.sh` from the repository root.
- Windows: run the equivalent `shadercross source -o output` commands in PowerShell for each source and target format.

The script compiles every `.vert.hlsl` and `.frag.hlsl` source into SPIR-V, MSL, and DXIL files under `shaders/Compiled`. Generated binaries are build artifacts and are not currently bundled into the application.

## Current integration status

The application still uses its CPU renderer for the 3D scene. The runtime GPU mode currently acquires the swapchain and clears it to a solid color; it does not yet load these shaders, create a graphics pipeline, upload mesh buffers, or draw the scene.

The next integration step is to compile/package these shaders for the active SDL_GPU backend, then create the graphics pipeline and upload the scene's mesh data. Depth testing and texture sampling are not yet integrated.
