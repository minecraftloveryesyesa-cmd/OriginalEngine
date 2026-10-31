#include "engine/GpuRenderer.hpp"

#include <cstring>

namespace originalengine {
namespace {

constexpr char vertexSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;
struct VertexOutput { float4 position [[position]]; float3 color; };
vertex VertexOutput main_vertex(uint vertexID [[vertex_id]]) {
    constexpr float2 positions[3] = {
        float2(0.0, 0.68), float2(-0.62, -0.48), float2(0.62, -0.48)
    };
    constexpr float3 colors[3] = {
        float3(1.0, 0.28, 0.18), float3(0.18, 0.78, 1.0), float3(0.42, 1.0, 0.48)
    };
    VertexOutput output;
    output.position = float4(positions[vertexID], 0.0, 1.0);
    output.color = colors[vertexID];
    return output;
}
)metal";

constexpr char fragmentSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;
struct VertexOutput { float4 position [[position]]; float3 color; };
fragment float4 main_fragment(VertexOutput input [[stage_in]]) {
    return float4(saturate(input.color), 1.0);
}
)metal";

} // namespace

GpuRenderer::~GpuRenderer() {
    shutdown();
}

void GpuRenderer::setError(const char* operation) {
    lastError_ = std::string(operation) + " failed: " + SDL_GetError();
}

bool GpuRenderer::initialize(SDL_Window* window) {
    shutdown();
    lastError_.clear();
    driverName_.clear();
    if (window == nullptr) {
        lastError_ = "Cannot initialize GPU renderer without a window";
        return false;
    }

    // The current shader source is MSL, so request the Metal shader format.
    device_ = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, false, nullptr);
    if (device_ == nullptr) {
        setError("SDL_CreateGPUDevice");
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device_, window)) {
        setError("SDL_ClaimWindowForGPUDevice");
        shutdown();
        return false;
    }
    window_ = window;
    const char* driver = SDL_GetGPUDeviceDriver(device_);
    driverName_ = driver != nullptr ? driver : "Unknown";

    SDL_GPUShaderCreateInfo vertexInfo{};
    vertexInfo.code_size = std::strlen(vertexSource);
    vertexInfo.code = reinterpret_cast<const Uint8*>(vertexSource);
    vertexInfo.entrypoint = "main_vertex";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(device_, &vertexInfo);
    if (vertexShader == nullptr) {
        setError("SDL_CreateGPUShader(vertex)");
        shutdown();
        return false;
    }

    SDL_GPUShaderCreateInfo fragmentInfo{};
    fragmentInfo.code_size = std::strlen(fragmentSource);
    fragmentInfo.code = reinterpret_cast<const Uint8*>(fragmentSource);
    fragmentInfo.entrypoint = "main_fragment";
    fragmentInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    fragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(device_, &fragmentInfo);
    if (fragmentShader == nullptr) {
        setError("SDL_CreateGPUShader(fragment)");
        SDL_ReleaseGPUShader(device_, vertexShader);
        shutdown();
        return false;
    }

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipelineInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.num_color_targets = 1;

    pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo);
    SDL_ReleaseGPUShader(device_, fragmentShader);
    SDL_ReleaseGPUShader(device_, vertexShader);
    if (pipeline_ == nullptr) {
        setError("SDL_CreateGPUGraphicsPipeline");
        shutdown();
        return false;
    }
    return true;
}

bool GpuRenderer::renderFrame() {
    if (device_ == nullptr || window_ == nullptr || pipeline_ == nullptr) {
        lastError_ = "GPU renderer is not initialized";
        return false;
    }

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device_);
    if (commandBuffer == nullptr) {
        setError("SDL_AcquireGPUCommandBuffer");
        return false;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commandBuffer, window_, &swapchainTexture, &width, &height)) {
        setError("SDL_WaitAndAcquireGPUSwapchainTexture");
        (void)SDL_CancelGPUCommandBuffer(commandBuffer);
        return false;
    }
    if (swapchainTexture == nullptr || width == 0 || height == 0) {
        if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
            setError("SDL_SubmitGPUCommandBuffer");
            return false;
        }
        return true;
    }

    SDL_GPUColorTargetInfo target{};
    target.texture = swapchainTexture;
    target.clear_color = SDL_FColor{0.025F, 0.12F, 0.19F, 1.0F};
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer, &target, 1, nullptr);
    if (pass == nullptr) {
        setError("SDL_BeginGPURenderPass");
        // The swapchain texture is already acquired; submit this command buffer.
        (void)SDL_SubmitGPUCommandBuffer(commandBuffer);
        return false;
    }

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        setError("SDL_SubmitGPUCommandBuffer");
        return false;
    }
    return true;
}

void GpuRenderer::shutdown() noexcept {
    if (device_ != nullptr) {
        if (pipeline_ != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
            pipeline_ = nullptr;
        }
        if (window_ != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(device_, window_);
            window_ = nullptr;
        }
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
    }
}

} // namespace originalengine
