#include "engine/Renderer3D.hpp"

#include "engine/Camera.hpp"
#include "engine/Mesh.hpp"
#include "engine/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace originalengine {
namespace {

struct Face {
    std::array<int, 4> indices;
    SDL_Color color;
};

Vec3 rotateX(Vec3 p, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}

Vec3 rotateY(Vec3 p, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}

Vec3 rotateZ(Vec3 p, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

Vec3 toCameraSpace(Vec3 world, const Camera& camera) {
    Vec3 p{world.x - camera.x(), world.y - camera.y(), world.z - camera.z()};
    p = rotateY(p, -camera.yaw());
    p = rotateX(p, camera.pitch());
    return p;
}

std::vector<Vec3> clipAgainstNearPlane(const std::vector<Vec3>& polygon, float nearPlane) {
    std::vector<Vec3> clipped;
    if (polygon.empty()) {
        return clipped;
    }

    Vec3 previous = polygon.back();
    bool previousInside = previous.z > nearPlane;
    for (const Vec3 current : polygon) {
        const bool currentInside = current.z > nearPlane;
        if (currentInside != previousInside) {
            const float denominator = current.z - previous.z;
            if (std::abs(denominator) > 1.0e-7F) {
                const float t = (nearPlane - previous.z) / denominator;
                clipped.push_back({
                    previous.x + (current.x - previous.x) * t,
                    previous.y + (current.y - previous.y) * t,
                    nearPlane
                });
            }
        }
        if (currentInside) {
            clipped.push_back(current);
        }
        previous = current;
        previousInside = currentInside;
    }
    return clipped;
}

SDL_Vertex makeVertex(Vec3 point, SDL_Color color,
                      const auto& project) {
    SDL_Vertex vertex{};
    vertex.position = project(point);
    vertex.color = {
        static_cast<float>(color.r) / 255.0F,
        static_cast<float>(color.g) / 255.0F,
        static_cast<float>(color.b) / 255.0F,
        static_cast<float>(color.a) / 255.0F
    };
    vertex.tex_coord = {0.0F, 0.0F};
    return vertex;
}


void rasterizeTriangle(
    int width, int height,
    const std::array<SDL_FPoint, 3>& points,
    const std::array<float, 3>& depths,
    SDL_Color color,
    std::vector<Uint8>& colorBuffer,
    std::vector<float>& depthBuffer) {
    const float area =
        (points[1].x - points[0].x) * (points[2].y - points[0].y) -
        (points[1].y - points[0].y) * (points[2].x - points[0].x);
    if (std::abs(area) < 1.0e-6F) {
        return;
    }

    const float minX = std::min({points[0].x, points[1].x, points[2].x});
    const float maxX = std::max({points[0].x, points[1].x, points[2].x});
    const float minY = std::min({points[0].y, points[1].y, points[2].y});
    const float maxY = std::max({points[0].y, points[1].y, points[2].y});
    const int left = std::max(0, static_cast<int>(std::floor(minX)));
    const int right = std::min(width - 1, static_cast<int>(std::ceil(maxX)));
    const int top = std::max(0, static_cast<int>(std::floor(minY)));
    const int bottom = std::min(height - 1, static_cast<int>(std::ceil(maxY)));
    if (left > right || top > bottom) {
        return;
    }

    const float invZ0 = 1.0F / depths[0];
    const float invZ1 = 1.0F / depths[1];
    const float invZ2 = 1.0F / depths[2];
    for (int y = top; y <= bottom; ++y) {
        for (int x = left; x <= right; ++x) {
            const float px = static_cast<float>(x) + 0.5F;
            const float py = static_cast<float>(y) + 0.5F;
            const float w0 = ((points[1].x - px) * (points[2].y - py) -
                              (points[1].y - py) * (points[2].x - px)) / area;
            const float w1 = ((points[2].x - px) * (points[0].y - py) -
                              (points[2].y - py) * (points[0].x - px)) / area;
            const float w2 = 1.0F - w0 - w1;
            if (w0 < -1.0e-5F || w1 < -1.0e-5F || w2 < -1.0e-5F) {
                continue;
            }

            const float reciprocalDepth = w0 * invZ0 + w1 * invZ1 + w2 * invZ2;
            if (reciprocalDepth <= 0.0F) {
                continue;
            }
            const float depth = 1.0F / reciprocalDepth;
            const std::size_t pixel = static_cast<std::size_t>(y) *
                                      static_cast<std::size_t>(width) +
                                      static_cast<std::size_t>(x);
            if (depth >= depthBuffer[pixel]) {
                continue;
            }
            depthBuffer[pixel] = depth;
            const std::size_t offset = pixel * 4U;
            colorBuffer[offset] = color.r;
            colorBuffer[offset + 1U] = color.g;
            colorBuffer[offset + 2U] = color.b;
            colorBuffer[offset + 3U] = color.a;
        }
    }
}


void rasterizeDepthLine(
    int width, int height,
    const SDL_FPoint& a, float depthA,
    const SDL_FPoint& b, float depthB,
    SDL_Color color,
    std::vector<Uint8>& colorBuffer,
    std::vector<float>& depthBuffer) {
    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const int steps = static_cast<int>(std::ceil(std::max(std::abs(dx), std::abs(dy))));
    if (steps <= 0) {
        return;
    }

    const float invDepthA = 1.0F / depthA;
    const float invDepthB = 1.0F / depthB;
    for (int step = 0; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        const int x = static_cast<int>(std::floor(a.x + dx * t));
        const int y = static_cast<int>(std::floor(a.y + dy * t));
        if (x < 0 || x >= width || y < 0 || y >= height) {
            continue;
        }

        const float reciprocalDepth = invDepthA + (invDepthB - invDepthA) * t;
        if (reciprocalDepth <= 0.0F) {
            continue;
        }
        const float depth = 1.0F / reciprocalDepth;
        const std::size_t pixel = static_cast<std::size_t>(y) *
                                  static_cast<std::size_t>(width) +
                                  static_cast<std::size_t>(x);
        if (depth >= depthBuffer[pixel]) {
            continue;
        }
        depthBuffer[pixel] = depth;
        const std::size_t offset = pixel * 4U;
        colorBuffer[offset] = color.r;
        colorBuffer[offset + 1U] = color.g;
        colorBuffer[offset + 2U] = color.b;
        colorBuffer[offset + 3U] = color.a;
    }
}

} // namespace

Renderer3D::Renderer3D(SDL_Renderer* renderer)
    : renderer_(renderer) {
    if (renderer_ == nullptr) {
        throw std::invalid_argument("Renderer3D requires a valid SDL_Renderer.");
    }
}

Renderer3D::~Renderer3D() {
    if (depthTexture_ != nullptr) {
        SDL_DestroyTexture(depthTexture_);
    }
}

bool Renderer3D::render(int width, int height, double elapsedSeconds,
                        const Camera& camera,
                        const std::vector<MeshInstance>& meshInstances, bool wireframe) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    if (previousElapsedSeconds_ > 0.0) {
        const double frameSeconds = elapsedSeconds - previousElapsedSeconds_;
        if (frameSeconds > 0.0001) {
            const float instantaneousFps = static_cast<float>(1.0 / frameSeconds);
            smoothedFramesPerSecond_ = smoothedFramesPerSecond_ <= 0.0F
                ? instantaneousFps
                : smoothedFramesPerSecond_ * 0.9F + instantaneousFps * 0.1F;
        }
    }
    previousElapsedSeconds_ = elapsedSeconds;

    SDL_SetRenderDrawColor(renderer_, 12, 18, 31, 255);
    SDL_RenderClear(renderer_);

    if (!wireframe && (depthTexture_ == nullptr ||
                       textureWidth_ != width || textureHeight_ != height)) {
        if (depthTexture_ != nullptr) {
            SDL_DestroyTexture(depthTexture_);
            depthTexture_ = nullptr;
        }
        depthTexture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING, width, height);
        if (depthTexture_ == nullptr) {
            SDL_Log("Could not create depth-buffer texture: %s", SDL_GetError());
            return false;
        }
        if (!SDL_SetTextureBlendMode(depthTexture_, SDL_BLENDMODE_BLEND)) {
            SDL_Log("Could not set depth-buffer texture blend mode: %s", SDL_GetError());
            return false;
        }
        textureWidth_ = width;
        textureHeight_ = height;
        colorBuffer_.resize(static_cast<std::size_t>(width) *
                            static_cast<std::size_t>(height) * 4U);
        depthBuffer_.resize(static_cast<std::size_t>(width) *
                            static_cast<std::size_t>(height));
    }
    if (!wireframe) {
        std::fill(colorBuffer_.begin(), colorBuffer_.end(), Uint8{0});
        std::fill(depthBuffer_.begin(), depthBuffer_.end(),
                  std::numeric_limits<float>::infinity());
    }

    const float scale = static_cast<float>(std::min(width, height)) * 0.42F;
    const float centerX = static_cast<float>(width) * 0.5F;
    const float centerY = static_cast<float>(height) * 0.5F;
    constexpr float focalLength = 2.4F;
    constexpr float nearPlane = 0.08F;

    auto project = [&](Vec3 p) -> SDL_FPoint {
        const float perspective = focalLength / p.z;
        return {centerX + p.x * scale * perspective,
                centerY - p.y * scale * perspective};
    };

    // Collect world-space floor-grid lines; filled mode depth-tests them with meshes.
    struct GridLine {
        SDL_FPoint a;
        SDL_FPoint b;
        float depthA;
        float depthB;
    };
    std::vector<GridLine> gridLines;
    auto addGridLine = [&](Vec3 a, Vec3 b) {
        Vec3 cameraA = toCameraSpace(a, camera);
        Vec3 cameraB = toCameraSpace(b, camera);
        if (cameraA.z <= nearPlane && cameraB.z <= nearPlane) {
            return;
        }
        if (cameraA.z <= nearPlane || cameraB.z <= nearPlane) {
            Vec3& outside = cameraA.z <= nearPlane ? cameraA : cameraB;
            const Vec3& inside = cameraA.z <= nearPlane ? cameraB : cameraA;
            const float t = (nearPlane - outside.z) / (inside.z - outside.z);
            outside = {
                outside.x + (inside.x - outside.x) * t,
                outside.y + (inside.y - outside.y) * t,
                nearPlane
            };
        }
        gridLines.push_back({project(cameraA), project(cameraB), cameraA.z, cameraB.z});
    };

    constexpr float floorY = -1.5F;
    for (int i = -20; i <= 20; ++i) {
        const float coordinate = static_cast<float>(i);
        addGridLine({coordinate, floorY, -20.0F}, {coordinate, floorY, 40.0F});
    }
    for (int i = -2; i <= 40; ++i) {
        const float z = static_cast<float>(i);
        addGridLine({-20.0F, floorY, z}, {20.0F, floorY, z});
    }

    const std::array<Vec3, 8> cube{{
        {-1.0F, -1.0F, -1.0F}, { 1.0F, -1.0F, -1.0F},
        { 1.0F,  1.0F, -1.0F}, {-1.0F,  1.0F, -1.0F},
        {-1.0F, -1.0F,  1.0F}, { 1.0F, -1.0F,  1.0F},
        { 1.0F,  1.0F,  1.0F}, {-1.0F,  1.0F,  1.0F}
    }};

    std::array<Vec3, 8> transformed{};
    const float time = static_cast<float>(elapsedSeconds);
    const float angleY = time * 0.65F;
    const float angleX = std::sin(time * 0.43F) * 0.28F;
    const float angleZ = std::sin(time * 0.31F) * 0.12F;

    for (std::size_t i = 0; i < cube.size(); ++i) {
        Vec3 p = rotateX(cube[i], angleX);
        p = rotateY(p, angleY);
        p = rotateZ(p, angleZ);
        p.z += 4.2F;
        transformed[i] = toCameraSpace(p, camera);
    }

    const std::array<Face, 6> baseFaces{{
        {{{0, 1, 2, 3}}, {235, 83, 75, 255}},
        {{{5, 4, 7, 6}}, {72, 165, 245, 255}},
        {{{4, 0, 3, 7}}, {74, 201, 157, 255}},
        {{{1, 5, 6, 2}}, {246, 190, 76, 255}},
        {{{3, 2, 6, 7}}, {171, 120, 235, 255}},
        {{{4, 5, 1, 0}}, {76, 207, 220, 255}}
    }};

    const std::array<Face, 6>& faces = baseFaces;
    for (const Face& face : faces) {
        std::vector<Vec3> polygon;
        polygon.reserve(face.indices.size());
        for (const int index : face.indices) {
            polygon.push_back(transformed[static_cast<std::size_t>(index)]);
        }
        polygon = clipAgainstNearPlane(polygon, nearPlane);
        if (polygon.size() < 3) {
            continue;
        }

        if (wireframe) {
            SDL_SetRenderDrawColor(renderer_, 220, 232, 250, 255);
            for (std::size_t i = 0; i < polygon.size(); ++i) {
                const SDL_FPoint a = project(polygon[i]);
                const SDL_FPoint b = project(polygon[(i + 1) % polygon.size()]);
                if (!SDL_RenderLine(renderer_, a.x, a.y, b.x, b.y)) {
                    SDL_Log("SDL_RenderLine for wireframe cube failed: %s", SDL_GetError());
                    return false;
                }
            }
        } else {
            for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
                const std::array<Vec3, 3> points{polygon[0], polygon[i], polygon[i + 1]};
                std::array<SDL_FPoint, 3> projected{};
                std::array<float, 3> depths{};
                for (std::size_t corner = 0; corner < 3; ++corner) {
                    projected[corner] = project(points[corner]);
                    depths[corner] = points[corner].z;
                }
                rasterizeTriangle(width, height, projected, depths, face.color,
                                  colorBuffer_, depthBuffer_);
            }
        }
    }

    // Draw every visible scene object that has a valid mesh and transform.
    for (const MeshInstance& instance : meshInstances) {
        if (instance.mesh == nullptr || instance.object == nullptr ||
            !instance.object->visible || instance.mesh->empty()) {
            continue;
        }
        const Mesh& mesh = *instance.mesh;
        const Transform& meshTransform = instance.object->transform;
        struct TriangleToDraw {
            std::array<SDL_Vertex, 3> vertices;
            std::array<float, 3> depths;
            SDL_Color color{};
        };
        std::vector<TriangleToDraw> triangles;
        triangles.reserve(mesh.triangles.size());

        for (const MeshTriangle& triangle : mesh.triangles) {
            std::vector<Vec3> polygon;
            polygon.reserve(3);
            bool valid = true;
            for (const std::size_t index : triangle.indices) {
                if (index >= mesh.vertices.size()) {
                    valid = false;
                    break;
                }
                const MeshVertex& vertex = mesh.vertices[index];
                Vec3 point{
                    vertex.x * meshTransform.scale.x,
                    vertex.y * meshTransform.scale.y,
                    vertex.z * meshTransform.scale.z
                };
                point = rotateX(point, meshTransform.rotation.x);
                point = rotateY(point, meshTransform.rotation.y);
                point = rotateZ(point, meshTransform.rotation.z);
                point.x += meshTransform.position.x;
                point.y += meshTransform.position.y;
                point.z += meshTransform.position.z;
                point = toCameraSpace(point, camera);
                polygon.push_back(point);
            }
            if (!valid) {
                continue;
            }
            polygon = clipAgainstNearPlane(polygon, nearPlane);
            if (polygon.size() < 3) {
                continue;
            }

            for (std::size_t i = 1; i + 1 < polygon.size(); ++i) {
                const Vec3 points[3] = {polygon[0], polygon[i], polygon[i + 1]};

                // Flat diffuse lighting from the triangle's camera-space face normal.
                const Vec3 edgeA{
                    points[1].x - points[0].x,
                    points[1].y - points[0].y,
                    points[1].z - points[0].z
                };
                const Vec3 edgeB{
                    points[2].x - points[0].x,
                    points[2].y - points[0].y,
                    points[2].z - points[0].z
                };
                Vec3 normal{
                    edgeA.y * edgeB.z - edgeA.z * edgeB.y,
                    edgeA.z * edgeB.x - edgeA.x * edgeB.z,
                    edgeA.x * edgeB.y - edgeA.y * edgeB.x
                };
                const float normalLength = std::sqrt(
                    normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                if (normalLength > 1.0e-6F) {
                    normal.x /= normalLength;
                    normal.y /= normalLength;
                    normal.z /= normalLength;
                }

                constexpr Vec3 lightDirection{0.35F, 0.75F, -0.55F};
                constexpr float lightLength = 0.99624294F;
                const float diffuse = std::abs(
                    normal.x * lightDirection.x +
                    normal.y * lightDirection.y +
                    normal.z * lightDirection.z) / lightLength;
                const float intensity = 0.24F + 0.76F * std::clamp(diffuse, 0.0F, 1.0F);

                TriangleToDraw item{};
                constexpr SDL_Color baseColor{72, 190, 238, 255};
                item.color = {
                    static_cast<Uint8>(static_cast<float>(baseColor.r) * intensity),
                    static_cast<Uint8>(static_cast<float>(baseColor.g) * intensity),
                    static_cast<Uint8>(static_cast<float>(baseColor.b) * intensity),
                    baseColor.a
                };
                for (std::size_t corner = 0; corner < 3; ++corner) {
                    item.vertices[corner] = makeVertex(points[corner], item.color, project);
                    item.depths[corner] = points[corner].z;
                }
                triangles.push_back(item);
            }
        }

        for (const TriangleToDraw& triangle : triangles) {
            if (wireframe) {
                SDL_SetRenderDrawColor(renderer_, 120, 220, 255, 255);
                for (std::size_t i = 0; i < triangle.vertices.size(); ++i) {
                    const SDL_FPoint a = triangle.vertices[i].position;
                    const SDL_FPoint b = triangle.vertices[(i + 1) % triangle.vertices.size()].position;
                    if (!SDL_RenderLine(renderer_, a.x, a.y, b.x, b.y)) {
                        SDL_Log("SDL_RenderLine for wireframe mesh failed: %s", SDL_GetError());
                        return false;
                    }
                }
            } else {
                std::array<SDL_FPoint, 3> projected{};
                std::array<float, 3> depths{};
                for (std::size_t corner = 0; corner < 3; ++corner) {
                    projected[corner] = triangle.vertices[corner].position;
                    depths[corner] = triangle.depths[corner];
                }
                rasterizeTriangle(width, height, projected, depths, triangle.color,
                                  colorBuffer_, depthBuffer_);
            }
        }
    }

    if (!wireframe) {
        const SDL_Color gridColor{31, 43, 61, 255};
        for (const GridLine& line : gridLines) {
            rasterizeDepthLine(width, height, line.a, line.depthA,
                               line.b, line.depthB, gridColor,
                               colorBuffer_, depthBuffer_);
        }
        if (!SDL_UpdateTexture(depthTexture_, nullptr, colorBuffer_.data(), width * 4)) {
            SDL_Log("SDL_UpdateTexture for depth buffer failed: %s", SDL_GetError());
            return false;
        }
        if (!SDL_RenderTexture(renderer_, depthTexture_, nullptr, nullptr)) {
            SDL_Log("SDL_RenderTexture for depth buffer failed: %s", SDL_GetError());
            return false;
        }
    }

    // SDL3's built-in debug font keeps the overlay dependency-free.
    SDL_SetRenderDrawColor(renderer_, 232, 239, 250, 255);
    char fpsText[64]{};
    char positionText[96]{};
    std::snprintf(fpsText, sizeof(fpsText), "FPS: %.1f", smoothedFramesPerSecond_);
    std::snprintf(positionText, sizeof(positionText),
                  "Camera: X %.2f  Y %.2f  Z %.2f",
                  static_cast<double>(camera.x()),
                  static_cast<double>(camera.y()),
                  static_cast<double>(camera.z()));
    (void)SDL_RenderDebugText(renderer_, 12.0F, 12.0F, fpsText);
    (void)SDL_RenderDebugText(renderer_, 12.0F, 28.0F, positionText);
    (void)SDL_RenderDebugText(renderer_, 12.0F, 44.0F,
                              "WASD Move | Q/E Up/Down | Shift Sprint");
    (void)SDL_RenderDebugText(renderer_, 12.0F, 60.0F,
                              "1/2 Toggle Pyramids | F3 Wireframe | Right-click Mouse Look | Esc Exit");
    if (wireframe) {
        (void)SDL_RenderDebugText(renderer_, 12.0F, 76.0F, "Wireframe: ON");
    }

    SDL_RenderPresent(renderer_);
    return true;
}

} // namespace originalengine
