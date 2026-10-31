#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "engine/Camera.hpp"
#include "engine/Mesh.hpp"
#include "engine/Scene.hpp"

namespace originalengine {

// SDL_GPU renderer for indexed triangle meshes and the shared scene camera.
// Mesh data is uploaded once during initialization; per-object MVP uniforms are
// pushed while recording each frame.
class GpuRenderer {
public:
    GpuRenderer() = default;
    ~GpuRenderer();

    GpuRenderer(const GpuRenderer&) = delete;
    GpuRenderer& operator=(const GpuRenderer&) = delete;

    [[nodiscard]] bool initialize(SDL_Window* window, const Mesh& mesh);
    [[nodiscard]] bool renderFrame(
        const Camera& camera, const std::vector<const SceneObject*>& objects);
    void shutdown() noexcept;

    [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }
    [[nodiscard]] const std::string& driverName() const noexcept { return driverName_; }

private:
    void setError(const char* operation);
    [[nodiscard]] bool createDepthTexture(Uint32 width, Uint32 height);

    SDL_Window* window_{nullptr};
    SDL_GPUDevice* device_{nullptr};
    SDL_GPUGraphicsPipeline* pipeline_{nullptr};
    SDL_GPUBuffer* vertexBuffer_{nullptr};
    SDL_GPUBuffer* indexBuffer_{nullptr};
    SDL_GPUTexture* depthTexture_{nullptr};
    SDL_GPUTextureFormat depthFormat_{SDL_GPU_TEXTUREFORMAT_INVALID};
    Uint32 depthWidth_{0};
    Uint32 depthHeight_{0};
    Uint32 indexCount_{0};
    std::string lastError_;
    std::string driverName_;
};

} // namespace originalengine
