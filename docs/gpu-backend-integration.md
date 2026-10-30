# SDL GPU backend integration notes

This document records the verified integration constraints for the SDL 3.2.10 GPU backend.

## Current application ownership

The sample application currently creates an `SDL_Renderer` for its window and uses that renderer for both the software 3D output and the touch-control overlay. The GPU capability probe is intentionally destroyed after it reports the available driver and shader formats.

An `SDL_GPUDevice` must claim the window with `SDL_ClaimWindowForGPUDevice` before it can acquire the window swapchain. The CPU renderer and GPU renderer must therefore not be treated as two independent presenters for the same window.

## Required backend transition

A real CPU/GPU switch must perform an ownership transition:

1. Stop issuing commands to the active backend.
2. Destroy the current `SDL_Renderer` before claiming the window for GPU rendering.
3. Create an `SDL_GPUDevice` with a shader format supported by the device.
4. Claim the window with `SDL_ClaimWindowForGPUDevice`.
5. Create GPU shaders, buffers, and a graphics pipeline.
6. Render through an acquired GPU command buffer and the window swapchain.
7. On switching back, release the window from the GPU device, destroy GPU resources/device, and recreate the `SDL_Renderer`.

The transition must handle every failure path. If GPU initialization fails after the CPU renderer has been released, recreate the CPU renderer before returning to CPU mode.

## Rendering milestones

### Milestone 1: GPU triangle

- Compile the HLSL vertex and fragment shaders with SDL_shadercross.
- Select SPIR-V, MSL, or DXIL based on `SDL_GetGPUShaderFormats`.
- Create a vertex buffer containing clip-space position and RGB color.
- Create a graphics pipeline for the window's swapchain color format.
- Acquire a command buffer and swapchain texture.
- Begin a color render pass, bind the pipeline and vertex buffer, draw three vertices, end the pass, and submit the command buffer.

This milestone should be a separate backend path; it should not be presented as a working CPU/GPU toggle until resource teardown and fallback are implemented.

### Milestone 2: scene integration

- Convert the existing camera and mesh output into GPU vertex data.
- Add depth testing and a depth texture.
- Add texture and sampler support.
- Preserve the existing CPU renderer as the fallback.

### Milestone 3: UI integration and switching

The current touch overlay uses SDL_Renderer drawing calls. GPU mode needs a GPU-compatible overlay pass (or a separately validated compositing strategy) before the CPU/GPU switch can be considered feature-complete.

## SDL 3.2.10 API contracts checked

The following declarations were checked against the SDL `release-3.2.10` `SDL_gpu.h` header:

- `SDL_CreateGPUDevice`
- `SDL_ClaimWindowForGPUDevice`
- `SDL_ReleaseWindowFromGPUDevice`
- `SDL_CreateGPUShader`
- `SDL_CreateGPUBuffer`
- `SDL_CreateGPUGraphicsPipeline`
- `SDL_AcquireGPUCommandBuffer`
- `SDL_WaitAndAcquireGPUSwapchainTexture`
- `SDL_BeginGPURenderPass`
- `SDL_BindGPUGraphicsPipeline`
- `SDL_BindGPUVertexBuffers`
- `SDL_DrawGPUPrimitives`
- `SDL_EndGPURenderPass`
- `SDL_SubmitGPUCommandBuffer`

This note is an implementation plan, not evidence that a GPU renderer currently works. No physical iPhone runtime test is implied.
