# SDL GPU shader sources

This directory contains the initial shared HLSL shader pair for the SDL GPU backend.

## Compile

Install the SDL_shadercross command-line tool and make sure `shadercross` is on PATH.

- macOS / Linux: run `sh shaders/compile.sh` from the repository root.
- Windows: run the equivalent `shadercross source -o output` commands in PowerShell for each source and each target format.

The script compiles every `.vert.hlsl` and `.frag.hlsl` source into SPIR-V, MSL, and DXIL files under `shaders/Compiled`. It stops on the first compiler error and verifies that each expected output is non-empty.

The script does not bundle SDL_shadercross or its dependencies. The generated files are build artifacts and should be regenerated when a source shader changes.

## Current status

The application does not load these files yet. The SDL GPU renderer, render pipeline, depth buffer, texture sampling, and CPU/GPU switching are not implemented.

The initial vertex shader accepts position and RGB color attributes and passes color to the fragment shader. Its vertex positions are expected to be supplied in clip space; camera transforms and mesh vertex-layout integration are future work.
