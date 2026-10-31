#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace originalengine {

// Owns the SDL_GPU device, window claim, pipeline, and frame submission.
// The initial implementation intentionally remains a triangle preview while
// the backend is separated from application input and CPU rendering.
class GpuRenderer {
public:
    GpuRenderer() = default;
    ~GpuRenderer();

    GpuRenderer(const GpuRenderer&) = delete;
    GpuRenderer& operator=(const GpuRenderer&) = delete;

    [[nodiscard]] bool initialize(SDL_Window* window);
    [[nodiscard]] bool renderFrame();
    void shutdown() noexcept;

    [[nodiscard]] const std::string& lastError() const noexcept { return lastError_; }
    [[nodiscard]] const std::string& driverName() const noexcept { return driverName_; }

private:
    void setError(const char* operation);

    SDL_Window* window_{nullptr};
    SDL_GPUDevice* device_{nullptr};
    SDL_GPUGraphicsPipeline* pipeline_{nullptr};
    std::string lastError_;
    std::string driverName_;
};

} // namespace originalengine
