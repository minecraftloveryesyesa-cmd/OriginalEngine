#include "engine/GpuDeviceProbe.hpp"

namespace originalengine {

GpuDeviceProbe::~GpuDeviceProbe() {
    shutdown();
}

bool GpuDeviceProbe::initialize(std::string* error) {
    shutdown();

    constexpr SDL_GPUShaderFormat formats =
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL;

    device_ = SDL_CreateGPUDevice(formats, false, nullptr);
    if (device_ == nullptr) {
        if (error != nullptr) {
            *error = SDL_GetError();
        }
        return false;
    }

    shaderFormats_ = SDL_GetGPUShaderFormats(device_);
    const char* driver = SDL_GetGPUDeviceDriver(device_);
    driverName_ = driver != nullptr ? driver : "unknown";

    if (shaderFormats_ == SDL_GPU_SHADERFORMAT_INVALID) {
        if (error != nullptr) {
            *error = "SDL_GPU device reported no supported shader formats";
        }
        shutdown();
        return false;
    }

    return true;
}

void GpuDeviceProbe::shutdown() {
    if (device_ != nullptr) {
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
    }
    shaderFormats_ = SDL_GPU_SHADERFORMAT_INVALID;
    driverName_.clear();
}

} // namespace originalengine
