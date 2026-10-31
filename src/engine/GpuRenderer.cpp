#include "engine/GpuRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <iterator>
#include <vector>

namespace originalengine {
namespace {

struct GpuVertex {
    float position[3];
    float normal[3];
    float color[3];
};

using Matrix4 = std::array<float, 16>;

constexpr char vertexSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct VertexInput {
    float3 position [[attribute(0)]];
    float3 normal [[attribute(1)]];
    float3 color [[attribute(2)]];
};

struct VertexOutput {
    float4 position [[position]];
    float3 normal;
    float3 color;
};

struct CameraConstants {
    float4x4 mvp;
};

vertex VertexOutput main_vertex(
    VertexInput input [[stage_in]],
    constant CameraConstants& camera [[buffer(0)]])
{
    VertexOutput output;
    output.position = camera.mvp * float4(input.position, 1.0);
    output.normal = input.normal;
    output.color = input.color;
    return output;
}
)metal";

constexpr char fragmentSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct VertexOutput {
    float4 position [[position]];
    float3 normal;
    float3 color;
};

fragment float4 main_fragment(VertexOutput input [[stage_in]])
{
    const float3 lightDirection = normalize(float3(-0.35, 0.8, -0.45));
    const float diffuse = max(dot(normalize(input.normal), lightDirection), 0.18);
    return float4(input.color * diffuse, 1.0);
}
)metal";


struct OverlayConstants {
    float center[2];
    float radius[2];
    float color[4];
    float viewport[2];
    Uint32 shape;
    Uint32 padding;
};

constexpr char overlayVertexSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;

struct OverlayConstants {
    float2 center;
    float2 radius;
    float4 color;
    float2 viewport;
    uint shape;
    uint padding;
};
struct OverlayOutput {
    float4 position [[position]];
    float4 color;
};
vertex OverlayOutput overlay_vertex(
    uint vertexId [[vertex_id]],
    constant OverlayConstants& constants [[buffer(0)]])
{
    OverlayOutput output;
    float2 local;
    if (constants.shape == 0) {
        const uint segment = vertexId / 6;
        const uint corner = vertexId % 6;
        const float angles[4] = {
            float(segment) / 32.0 * 6.28318530718,
            float(segment) / 32.0 * 6.28318530718,
            float(segment + 1) / 32.0 * 6.28318530718,
            float(segment) / 32.0 * 6.28318530718
        };
        const float angle0 = angles[0];
        const float angle1 = angles[2];
        const float2 outer0 = float2(cos(angle0), sin(angle0));
        const float2 inner0 = outer0 * 0.82;
        const float2 outer1 = float2(cos(angle1), sin(angle1));
        const float2 inner1 = outer1 * 0.82;
        if (corner == 0 || corner == 3) local = outer0;
        else if (corner == 1) local = inner0;
        else if (corner == 2 || corner == 4) local = inner1;
        else local = outer1;
    } else {
        const float2 quad[6] = {
            float2(-1.0, -1.0), float2(1.0, -1.0), float2(1.0, 1.0),
            float2(-1.0, -1.0), float2(1.0, 1.0), float2(-1.0, 1.0)
        };
        local = quad[vertexId];
    }
    const float2 pixel = constants.center + local * constants.radius;
    output.position = float4(
        pixel.x / constants.viewport.x * 2.0 - 1.0,
        1.0 - pixel.y / constants.viewport.y * 2.0, 0.0, 1.0);
    output.color = constants.color;
    return output;
}
)metal";

constexpr char overlayFragmentSource[] = R"metal(
#include <metal_stdlib>
using namespace metal;
struct OverlayOutput {
    float4 position [[position]];
    float4 color;
};
fragment float4 overlay_fragment(OverlayOutput input [[stage_in]])
{
    return input.color;
}
)metal";

Matrix4 identityMatrix() {
    Matrix4 result{};
    result[0] = 1.0F;
    result[5] = 1.0F;
    result[10] = 1.0F;
    result[15] = 1.0F;
    return result;
}

Matrix4 multiply(const Matrix4& a, const Matrix4& b) {
    Matrix4 result{};
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            for (int k = 0; k < 4; ++k) {
                result[static_cast<std::size_t>(column * 4 + row)] +=
                    a[static_cast<std::size_t>(k * 4 + row)] *
                    b[static_cast<std::size_t>(column * 4 + k)];
            }
        }
    }
    return result;
}

Matrix4 translationMatrix(const Vec3& position) {
    Matrix4 result = identityMatrix();
    result[12] = position.x;
    result[13] = position.y;
    result[14] = position.z;
    return result;
}

Matrix4 scaleMatrix(const Vec3& scale) {
    Matrix4 result{};
    result[0] = scale.x;
    result[5] = scale.y;
    result[10] = scale.z;
    result[15] = 1.0F;
    return result;
}

Matrix4 rotationX(float angle) {
    Matrix4 result = identityMatrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    result[5] = c;
    result[9] = -s;
    result[6] = s;
    result[10] = c;
    return result;
}

Matrix4 rotationY(float angle) {
    Matrix4 result = identityMatrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    result[0] = c;
    result[8] = s;
    result[2] = -s;
    result[10] = c;
    return result;
}

Matrix4 rotationZ(float angle) {
    Matrix4 result = identityMatrix();
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    result[0] = c;
    result[4] = -s;
    result[1] = s;
    result[5] = c;
    return result;
}

Matrix4 modelMatrix(const Transform& transform) {
    const Matrix4 rotations = multiply(
        rotationZ(transform.rotation.z),
        multiply(rotationY(transform.rotation.y), rotationX(transform.rotation.x)));
    return multiply(translationMatrix(transform.position),
                    multiply(rotations, scaleMatrix(transform.scale)));
}

Matrix4 viewMatrix(const Camera& camera) {
    const float yaw = camera.yaw();
    const float pitch = camera.pitch();
    const float sinYaw = std::sin(yaw);
    const float cosYaw = std::cos(yaw);
    const float sinPitch = std::sin(pitch);
    const float cosPitch = std::cos(pitch);

    const Vec3 right{cosYaw, 0.0F, -sinYaw};
    const Vec3 up{-sinYaw * sinPitch, cosPitch, -cosYaw * sinPitch};
    const Vec3 forward{sinYaw * cosPitch, sinPitch, cosYaw * cosPitch};
    const Vec3 position{camera.x(), camera.y(), camera.z()};

    Matrix4 result = identityMatrix();
    result[0] = right.x;   result[4] = right.y;   result[8] = right.z;
    result[1] = up.x;      result[5] = up.y;      result[9] = up.z;
    result[2] = forward.x; result[6] = forward.y; result[10] = forward.z;
    result[12] = -(right.x * position.x + right.y * position.y + right.z * position.z);
    result[13] = -(up.x * position.x + up.y * position.y + up.z * position.z);
    result[14] = -(forward.x * position.x + forward.y * position.y + forward.z * position.z);
    return result;
}

Matrix4 projectionMatrix(float aspectRatio) {
    constexpr float nearPlane = 0.1F;
    constexpr float farPlane = 100.0F;
    constexpr float verticalFov = 1.0471975512F;
    const float focalLength = 1.0F / std::tan(verticalFov * 0.5F);

    Matrix4 result{};
    result[0] = focalLength / std::max(aspectRatio, 0.001F);
    result[5] = focalLength;
    result[10] = farPlane / (farPlane - nearPlane);
    result[11] = 1.0F;
    result[14] = -(nearPlane * farPlane) / (farPlane - nearPlane);
    return result;
}

bool buildGpuGeometry(const Mesh& mesh, std::vector<GpuVertex>& vertices,
                      std::vector<std::uint32_t>& indices, std::string& error) {
    if (mesh.empty()) {
        error = "Cannot upload an empty mesh";
        return false;
    }

    vertices.clear();
    indices.clear();
    if (mesh.triangles.size() > std::numeric_limits<std::uint32_t>::max() / 3U) {
        error = "Mesh has too many triangle indices for SDL_GPU";
        return false;
    }
    vertices.reserve(mesh.triangles.size() * 3U);
    indices.reserve(mesh.triangles.size() * 3U);

    for (const MeshTriangle& triangle : mesh.triangles) {
        const std::size_t ia = triangle.indices[0];
        const std::size_t ib = triangle.indices[1];
        const std::size_t ic = triangle.indices[2];
        if (ia >= mesh.vertices.size() || ib >= mesh.vertices.size() ||
            ic >= mesh.vertices.size()) {
            error = "Mesh triangle contains an out-of-range vertex index";
            return false;
        }

        const MeshVertex& a = mesh.vertices[ia];
        const MeshVertex& b = mesh.vertices[ib];
        const MeshVertex& c = mesh.vertices[ic];
        const float abx = b.x - a.x;
        const float aby = b.y - a.y;
        const float abz = b.z - a.z;
        const float acx = c.x - a.x;
        const float acy = c.y - a.y;
        const float acz = c.z - a.z;
        const float nx = aby * acz - abz * acy;
        const float ny = abz * acx - abx * acz;
        const float nz = abx * acy - aby * acx;
        const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length <= 1.0e-7F) {
            continue;
        }

        const float normal[3]{nx / length, ny / length, nz / length};
        const float color[3]{
            0.30F + std::abs(normal[0]) * 0.45F,
            0.36F + std::abs(normal[1]) * 0.42F,
            0.48F + std::abs(normal[2]) * 0.40F
        };
        const MeshVertex* corners[3]{&a, &b, &c};
        for (const MeshVertex* corner : corners) {
            GpuVertex vertex{};
            vertex.position[0] = corner->x;
            vertex.position[1] = corner->y;
            vertex.position[2] = corner->z;
            std::copy(std::begin(normal), std::end(normal), std::begin(vertex.normal));
            std::copy(std::begin(color), std::end(color), std::begin(vertex.color));
            const auto index = static_cast<std::uint32_t>(vertices.size());
            vertices.push_back(vertex);
            indices.push_back(index);
        }
    }

    if (indices.empty()) {
        error = "Mesh contains no non-degenerate triangles";
        return false;
    }
    if (vertices.size() > std::numeric_limits<Uint32>::max() / sizeof(GpuVertex) ||
        indices.size() > std::numeric_limits<Uint32>::max() / sizeof(std::uint32_t)) {
        error = "Mesh GPU buffers exceed SDL_GPU buffer size limits";
        return false;
    }
    return true;
}

bool uploadBuffer(SDL_GPUDevice* device, SDL_GPUBuffer* destination,
                  const void* data, Uint32 size, std::string& error) {
    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = size;
    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (transfer == nullptr) {
        error = std::string("SDL_CreateGPUTransferBuffer failed: ") + SDL_GetError();
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    if (mapped == nullptr) {
        error = std::string("SDL_MapGPUTransferBuffer failed: ") + SDL_GetError();
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    std::memcpy(mapped, data, size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr) {
        error = std::string("SDL_AcquireGPUCommandBuffer failed: ") + SDL_GetError();
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    if (copyPass == nullptr) {
        error = std::string("SDL_BeginGPUCopyPass failed: ") + SDL_GetError();
        (void)SDL_CancelGPUCommandBuffer(commandBuffer);
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = transfer;
    SDL_GPUBufferRegion target{};
    target.buffer = destination;
    target.size = size;
    SDL_UploadToGPUBuffer(copyPass, &source, &target, false);
    SDL_EndGPUCopyPass(copyPass);

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        error = std::string("SDL_SubmitGPUCommandBuffer failed: ") + SDL_GetError();
        SDL_ReleaseGPUTransferBuffer(device, transfer);
        return false;
    }
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return true;
}

} // namespace

GpuRenderer::~GpuRenderer() {
    shutdown();
}

void GpuRenderer::setError(const char* operation) {
    lastError_ = std::string(operation) + " failed: " + SDL_GetError();
}

bool GpuRenderer::initialize(SDL_Window* window, const Mesh& mesh) {
    shutdown();
    lastError_.clear();
    driverName_.clear();
    if (window == nullptr) {
        lastError_ = "Cannot initialize GPU renderer without a window";
        return false;
    }

    std::vector<GpuVertex> vertices;
    std::vector<std::uint32_t> indices;
    if (!buildGpuGeometry(mesh, vertices, indices, lastError_)) {
        return false;
    }

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

    if (SDL_GPUTextureSupportsFormat(device_, SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
            SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        depthFormat_ = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    } else if (SDL_GPUTextureSupportsFormat(device_, SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
            SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        depthFormat_ = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    } else {
        lastError_ = "GPU device does not support a usable depth-stencil texture format";
        shutdown();
        return false;
    }

    SDL_GPUBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = static_cast<Uint32>(vertices.size() * sizeof(GpuVertex));
    vertexBuffer_ = SDL_CreateGPUBuffer(device_, &vertexBufferInfo);
    if (vertexBuffer_ == nullptr) {
        setError("SDL_CreateGPUBuffer(vertex)");
        shutdown();
        return false;
    }
    if (!uploadBuffer(device_, vertexBuffer_, vertices.data(), vertexBufferInfo.size, lastError_)) {
        shutdown();
        return false;
    }

    SDL_GPUBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferInfo.size = static_cast<Uint32>(indices.size() * sizeof(std::uint32_t));
    indexBuffer_ = SDL_CreateGPUBuffer(device_, &indexBufferInfo);
    if (indexBuffer_ == nullptr) {
        setError("SDL_CreateGPUBuffer(index)");
        shutdown();
        return false;
    }
    if (!uploadBuffer(device_, indexBuffer_, indices.data(), indexBufferInfo.size, lastError_)) {
        shutdown();
        return false;
    }
    indexCount_ = static_cast<Uint32>(indices.size());

    SDL_GPUShaderCreateInfo vertexInfo{};
    vertexInfo.code_size = std::strlen(vertexSource);
    vertexInfo.code = reinterpret_cast<const Uint8*>(vertexSource);
    vertexInfo.entrypoint = "main_vertex";
    vertexInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    vertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexInfo.num_uniform_buffers = 1;
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

    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = static_cast<Uint32>(sizeof(GpuVertex));
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    const SDL_GPUVertexAttribute attributes[]{
        {0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, static_cast<Uint32>(offsetof(GpuVertex, position))},
        {1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, static_cast<Uint32>(offsetof(GpuVertex, normal))},
        {2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3, static_cast<Uint32>(offsetof(GpuVertex, color))}
    };

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDescription;
    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_attributes = attributes;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 3;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipelineInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    pipelineInfo.depth_stencil_state.enable_depth_test = true;
    pipelineInfo.depth_stencil_state.enable_depth_write = true;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.has_depth_stencil_target = true;
    pipelineInfo.target_info.depth_stencil_format = depthFormat_;

    pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo);
    SDL_ReleaseGPUShader(device_, fragmentShader);
    SDL_ReleaseGPUShader(device_, vertexShader);
    if (pipeline_ == nullptr) {
        setError("SDL_CreateGPUGraphicsPipeline");
        shutdown();
        return false;
    }

    SDL_GPUShaderCreateInfo overlayVertexInfo{};
    overlayVertexInfo.code_size = std::strlen(overlayVertexSource);
    overlayVertexInfo.code = reinterpret_cast<const Uint8*>(overlayVertexSource);
    overlayVertexInfo.entrypoint = "overlay_vertex";
    overlayVertexInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    overlayVertexInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    overlayVertexInfo.num_uniform_buffers = 1;
    SDL_GPUShader* overlayVertexShader = SDL_CreateGPUShader(device_, &overlayVertexInfo);
    if (overlayVertexShader == nullptr) {
        setError("SDL_CreateGPUShader(overlay vertex)");
        shutdown();
        return false;
    }

    SDL_GPUShaderCreateInfo overlayFragmentInfo{};
    overlayFragmentInfo.code_size = std::strlen(overlayFragmentSource);
    overlayFragmentInfo.code = reinterpret_cast<const Uint8*>(overlayFragmentSource);
    overlayFragmentInfo.entrypoint = "overlay_fragment";
    overlayFragmentInfo.format = SDL_GPU_SHADERFORMAT_MSL;
    overlayFragmentInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    SDL_GPUShader* overlayFragmentShader = SDL_CreateGPUShader(device_, &overlayFragmentInfo);
    if (overlayFragmentShader == nullptr) {
        setError("SDL_CreateGPUShader(overlay fragment)");
        SDL_ReleaseGPUShader(device_, overlayVertexShader);
        shutdown();
        return false;
    }

    SDL_GPUColorTargetDescription overlayColorTarget{};
    overlayColorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
    SDL_GPUGraphicsPipelineCreateInfo overlayPipelineInfo{};
    overlayPipelineInfo.vertex_shader = overlayVertexShader;
    overlayPipelineInfo.fragment_shader = overlayFragmentShader;
    overlayPipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    overlayPipelineInfo.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
    overlayPipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    overlayPipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    overlayPipelineInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;
    overlayPipelineInfo.depth_stencil_state.enable_depth_test = false;
    overlayPipelineInfo.depth_stencil_state.enable_depth_write = false;
    overlayPipelineInfo.target_info.color_target_descriptions = &overlayColorTarget;
    overlayPipelineInfo.target_info.num_color_targets = 1;
    overlayPipelineInfo.target_info.has_depth_stencil_target = true;
    overlayPipelineInfo.target_info.depth_stencil_format = depthFormat_;
    overlayPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &overlayPipelineInfo);
    SDL_ReleaseGPUShader(device_, overlayFragmentShader);
    SDL_ReleaseGPUShader(device_, overlayVertexShader);
    if (overlayPipeline_ == nullptr) {
        setError("SDL_CreateGPUGraphicsPipeline(overlay)");
        shutdown();
        return false;
    }
    return true;
}

bool GpuRenderer::createDepthTexture(Uint32 width, Uint32 height) {
    if (depthTexture_ != nullptr && depthWidth_ == width && depthHeight_ == height) {
        return true;
    }
    if (depthTexture_ != nullptr) {
        SDL_ReleaseGPUTexture(device_, depthTexture_);
        depthTexture_ = nullptr;
    }

    SDL_GPUTextureCreateInfo textureInfo{};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = depthFormat_;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    textureInfo.width = width;
    textureInfo.height = height;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    depthTexture_ = SDL_CreateGPUTexture(device_, &textureInfo);
    if (depthTexture_ == nullptr) {
        depthWidth_ = 0;
        depthHeight_ = 0;
        setError("SDL_CreateGPUTexture(depth)");
        return false;
    }
    depthWidth_ = width;
    depthHeight_ = height;
    return true;
}

bool GpuRenderer::renderFrame(
    const Camera& camera, const std::vector<const SceneObject*>& objects,
    bool showTouchControls, bool movementActive, float movementX, float movementY) {
    if (device_ == nullptr || window_ == nullptr || pipeline_ == nullptr ||
        vertexBuffer_ == nullptr || indexBuffer_ == nullptr) {
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
    if (!createDepthTexture(width, height)) {
        (void)SDL_SubmitGPUCommandBuffer(commandBuffer);
        return false;
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = SDL_FColor{0.025F, 0.12F, 0.19F, 1.0F};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget{};
    depthTarget.texture = depthTexture_;
    depthTarget.clear_depth = 1.0F;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTarget, 1, &depthTarget);
    if (pass == nullptr) {
        setError("SDL_BeginGPURenderPass");
        (void)SDL_SubmitGPUCommandBuffer(commandBuffer);
        return false;
    }

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);
    const SDL_GPUBufferBinding vertexBinding{vertexBuffer_, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);
    const SDL_GPUBufferBinding indexBinding{indexBuffer_, 0};
    SDL_BindGPUIndexBuffer(pass, &indexBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);

    const Matrix4 view = viewMatrix(camera);
    const Matrix4 projection = projectionMatrix(
        static_cast<float>(width) / static_cast<float>(height));
    const Matrix4 viewProjection = multiply(projection, view);

    for (const SceneObject* object : objects) {
        if (object == nullptr || !object->visible) {
            continue;
        }
        const Matrix4 mvp = multiply(viewProjection, modelMatrix(object->transform));
        SDL_PushGPUVertexUniformData(commandBuffer, 0, mvp.data(),
                                     static_cast<Uint32>(sizeof(mvp)));
        SDL_DrawGPUIndexedPrimitives(pass, indexCount_, 1, 0, 0, 0);
    }

    if (showTouchControls && overlayPipeline_ != nullptr) {
        drawOverlay(commandBuffer, pass, width, height, movementActive, movementX, movementY);
    }

    SDL_EndGPURenderPass(pass);
    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        setError("SDL_SubmitGPUCommandBuffer");
        return false;
    }
    return true;
}


void GpuRenderer::drawOverlayRect(
    SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* pass,
    float x, float y, float rectWidth, float rectHeight,
    Uint32 viewportWidth, Uint32 viewportHeight, const float color[4]) {
    OverlayConstants constants{};
    constants.center[0] = x + rectWidth * 0.5F;
    constants.center[1] = y + rectHeight * 0.5F;
    constants.radius[0] = rectWidth * 0.5F;
    constants.radius[1] = rectHeight * 0.5F;
    std::copy_n(color, 4, constants.color);
    constants.viewport[0] = static_cast<float>(viewportWidth);
    constants.viewport[1] = static_cast<float>(viewportHeight);
    constants.shape = 1;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &constants,
                                 static_cast<Uint32>(sizeof(constants)));
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
}

void GpuRenderer::drawOverlayCircle(
    SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* pass,
    float x, float y, float radiusX, float radiusY,
    Uint32 viewportWidth, Uint32 viewportHeight, const float color[4]) {
    OverlayConstants constants{};
    constants.center[0] = x;
    constants.center[1] = y;
    constants.radius[0] = radiusX;
    constants.radius[1] = radiusY;
    std::copy_n(color, 4, constants.color);
    constants.viewport[0] = static_cast<float>(viewportWidth);
    constants.viewport[1] = static_cast<float>(viewportHeight);
    constants.shape = 0;
    SDL_PushGPUVertexUniformData(commandBuffer, 0, &constants,
                                 static_cast<Uint32>(sizeof(constants)));
    SDL_DrawGPUPrimitives(pass, 32U * 6U, 1, 0, 0);
}

void GpuRenderer::drawOverlay(
    SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* pass,
    Uint32 width, Uint32 height, bool movementActive, float movementX, float movementY) {
    SDL_BindGPUGraphicsPipeline(pass, overlayPipeline_);
    const float dimension = static_cast<float>(std::min(width, height));
    const float joystickX = static_cast<float>(width) * 0.20F;
    const float joystickY = static_cast<float>(height) * 0.78F;
    const float joystickRadius = dimension * 0.14F;
    const float ringColor[4]{0.82F, 0.88F, 0.96F, 1.0F};
    const float knobColor[4]{0.40F, 0.68F, 0.94F, 1.0F};
    const float buttonColor[4]{0.11F, 0.44F, 0.63F, 1.0F};
    const float textColor[4]{0.96F, 0.98F, 1.0F, 1.0F};

    drawOverlayCircle(commandBuffer, pass, joystickX, joystickY,
                      joystickRadius, joystickRadius, width, height, ringColor);
    drawOverlayCircle(commandBuffer, pass, joystickX, joystickY,
                      joystickRadius * 0.38F, joystickRadius * 0.38F,
                      width, height, ringColor);
    if (movementActive) {
        const float knobX = joystickX + std::clamp(
            movementX * static_cast<float>(width) - joystickX,
            -joystickRadius, joystickRadius);
        const float knobY = joystickY + std::clamp(
            movementY * static_cast<float>(height) - joystickY,
            -joystickRadius, joystickRadius);
        drawOverlayCircle(commandBuffer, pass, knobX, knobY,
                          joystickRadius * 0.25F, joystickRadius * 0.25F,
                          width, height, knobColor);
    }
    drawOverlayCircle(commandBuffer, pass, static_cast<float>(width) * 0.82F,
                      static_cast<float>(height) * 0.78F,
                      dimension * 0.12F, dimension * 0.12F,
                      width, height, ringColor);

    const float buttonX = static_cast<float>(width) - 116.0F;
    drawOverlayRect(commandBuffer, pass, buttonX, 18.0F, 98.0F, 42.0F,
                    width, height, buttonColor);

    // Tiny 3x5 bitmap font keeps the backend-switch label available without
    // depending on SDL_Renderer or a separate font subsystem.
    const char* glyphs[3] = {"111/101/110/101/111", "101/101/101/101/111",
                             "110/101/110/100/100"};
    const float textWidth = 3.0F * 3.0F + 2.0F * 2.0F;
    const float textStartX = buttonX + (98.0F - textWidth) * 0.5F;
    const float textStartY = 31.0F;
    for (int letter = 0; letter < 3; ++letter) {
        int row = 0;
        int column = 0;
        for (const char* cursor = glyphs[letter]; *cursor != '\0'; ++cursor) {
            if (*cursor == '/') {
                ++row;
                column = 0;
            } else {
                if (*cursor == '1') {
                    drawOverlayRect(commandBuffer, pass,
                        textStartX + static_cast<float>(letter) * 13.0F +
                            static_cast<float>(column) * 3.0F,
                        textStartY + static_cast<float>(row) * 2.0F,
                        2.0F, 1.6F, width, height, textColor);
                }
                ++column;
            }
        }
    }
}

void GpuRenderer::shutdown() noexcept {
    if (device_ != nullptr) {
        if (depthTexture_ != nullptr) {
            SDL_ReleaseGPUTexture(device_, depthTexture_);
            depthTexture_ = nullptr;
        }
        depthWidth_ = 0;
        depthHeight_ = 0;
        if (overlayPipeline_ != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(device_, overlayPipeline_);
            overlayPipeline_ = nullptr;
        }
        if (pipeline_ != nullptr) {
            SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
            pipeline_ = nullptr;
        }
        if (vertexBuffer_ != nullptr) {
            SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
            vertexBuffer_ = nullptr;
        }
        if (indexBuffer_ != nullptr) {
            SDL_ReleaseGPUBuffer(device_, indexBuffer_);
            indexBuffer_ = nullptr;
        }
        if (window_ != nullptr) {
            SDL_ReleaseWindowFromGPUDevice(device_, window_);
            window_ = nullptr;
        }
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
    }
    depthFormat_ = SDL_GPU_TEXTUREFORMAT_INVALID;
    indexCount_ = 0;
}

} // namespace originalengine
