# SDL GPU shader sources

This directory contains the first shared HLSL shader pair for the SDL GPU backend.

## Compile

Install the SDL_shadercross command-line tool, then run:

- macOS / Linux: `sh shaders/compile.sh`
- Windows: run the equivalent `shadercross` commands for each file in `shaders/Source`

The script writes SPIR-V, MSL, and DXIL variants under `shaders/Compiled`.

## Current status

These are shader-source foundations only. The application does not load these files yet, and the GPU renderer, render pipeline, depth buffer, texture sampling, and CPU/GPU switching are not implemented by this commit.

The initial vertex shader accepts position and RGB color attributes and passes color to the fragment shader. Its vertex positions are expected to be supplied in clip space; camera transforms and mesh vertex-layout integration are future work.
