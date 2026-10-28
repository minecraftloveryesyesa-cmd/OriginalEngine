#include "engine/Renderer3D.hpp"

#include "engine/Camera.hpp"
#include "engine/Mesh.hpp"
#include "engine/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <utility>
#include <vector>

namespace originalengine {
namespace {

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Face {
    std::array<int, 4> indices;
    SDL_Color color;
    float depth{};
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

} // namespace

Renderer3D::Renderer3D(SDL_Renderer* renderer)
    : renderer_(renderer) {
    if (renderer_ == nullptr) {
        throw std::invalid_argument("Renderer3D requires a valid SDL_Renderer.");
    }
}

bool Renderer3D::render(int width, int height, double elapsedSeconds,
                        const Camera& camera, const Mesh* mesh,
                        const Transform* meshTransform) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    SDL_SetRenderDrawColor(renderer_, 12, 18, 31, 255);
    SDL_RenderClear(renderer_);

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

    // Render a world-space floor grid, so it responds naturally to camera movement.
    SDL_SetRenderDrawColor(renderer_, 31, 43, 61, 255);
    auto drawGridLine = [&](Vec3 a, Vec3 b) {
        const Vec3 cameraA = toCameraSpace(a, camera);
        const Vec3 cameraB = toCameraSpace(b, camera);
        if (cameraA.z <= nearPlane || cameraB.z <= nearPlane) {
            return;
        }
        const SDL_FPoint pa = project(cameraA);
        const SDL_FPoint pb = project(cameraB);
        SDL_RenderLine(renderer_, pa.x, pa.y, pb.x, pb.y);
    };

    constexpr float floorY = -1.5F;
    for (int i = -20; i <= 20; ++i) {
        const float coordinate = static_cast<float>(i) * 1.0F;
        drawGridLine({coordinate, floorY, -20.0F},
                     {coordinate, floorY, 40.0F});
    }
    for (int i = -2; i <= 40; ++i) {
        const float z = static_cast<float>(i);
        drawGridLine({-20.0F, floorY, z}, {20.0F, floorY, z});
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

    std::array<Face, 6> faces = baseFaces;
    for (Face& face : faces) {
        face.depth = 0.0F;
        for (const int index : face.indices) {
            face.depth += transformed[static_cast<std::size_t>(index)].z * 0.25F;
        }
    }

    // Painter's algorithm: draw distant faces before nearer faces.
    std::sort(faces.begin(), faces.end(),
              [](const Face& a, const Face& b) { return a.depth > b.depth; });

    for (const Face& face : faces) {
        bool fullyInFront = true;
        for (const int index : face.indices) {
            if (transformed[static_cast<std::size_t>(index)].z <= nearPlane) {
                fullyInFront = false;
                break;
            }
        }
        if (!fullyInFront) {
            continue;
        }

        std::array<SDL_Vertex, 6> vertices{};
        const std::array<int, 6> triangleIndices{{
            face.indices[0], face.indices[1], face.indices[2],
            face.indices[0], face.indices[2], face.indices[3]
        }};

        for (std::size_t i = 0; i < triangleIndices.size(); ++i) {
            const Vec3 point = transformed[static_cast<std::size_t>(triangleIndices[i])];
            vertices[i].position = project(point);
            vertices[i].color = {
                static_cast<float>(face.color.r) / 255.0F,
                static_cast<float>(face.color.g) / 255.0F,
                static_cast<float>(face.color.b) / 255.0F,
                static_cast<float>(face.color.a) / 255.0F
            };
            vertices[i].tex_coord = {0.0F, 0.0F};
        }

        if (!SDL_RenderGeometry(renderer_, nullptr, vertices.data(),
                                static_cast<int>(vertices.size()), nullptr, 0)) {
            SDL_Log("SDL_RenderGeometry failed: %s", SDL_GetError());
            return false;
        }
    }

    // Draw an optional loaded mesh using the supplied scene transform.
    if (mesh != nullptr && meshTransform != nullptr && !mesh->empty()) {
        struct TriangleToDraw {
            std::array<SDL_Vertex, 3> vertices;
            float depth;
        };
        std::vector<TriangleToDraw> triangles;
        triangles.reserve(mesh->triangles.size());

        for (const MeshTriangle& triangle : mesh->triangles) {
            TriangleToDraw item{};
            bool valid = true;
            float depth = 0.0F;
            for (std::size_t corner = 0; corner < 3; ++corner) {
                const std::size_t index = triangle.indices[corner];
                if (index >= mesh->vertices.size()) {
                    valid = false;
                    break;
                }
                const MeshVertex& vertex = mesh->vertices[index];
                Vec3 point{
                    vertex.x * meshTransform->scale.x,
                    vertex.y * meshTransform->scale.y,
                    vertex.z * meshTransform->scale.z
                };
                point = rotateX(point, meshTransform->rotation.x);
                point = rotateY(point, meshTransform->rotation.y);
                point = rotateZ(point, meshTransform->rotation.z);
                point.x += meshTransform->position.x;
                point.y += meshTransform->position.y;
                point.z += meshTransform->position.z;
                point = toCameraSpace(point, camera);
                if (point.z <= nearPlane) {
                    valid = false;
                    break;
                }

                const SDL_FPoint screen = project(point);
                item.vertices[corner].position = screen;
                item.vertices[corner].color = {
                    0.25F + 0.12F * static_cast<float>(corner),
                    0.72F,
                    0.94F - 0.12F * static_cast<float>(corner),
                    1.0F
                };
                item.vertices[corner].tex_coord = {0.0F, 0.0F};
                depth += point.z / 3.0F;
            }
            if (valid) {
                item.depth = depth;
                triangles.push_back(item);
            }
        }

        std::sort(triangles.begin(), triangles.end(),
                  [](const TriangleToDraw& a, const TriangleToDraw& b) {
                      return a.depth > b.depth;
                  });
        for (const TriangleToDraw& triangle : triangles) {
            if (!SDL_RenderGeometry(renderer_, nullptr, triangle.vertices.data(), 3,
                                    nullptr, 0)) {
                SDL_Log("SDL_RenderGeometry for mesh failed: %s", SDL_GetError());
                return false;
            }
        }
    }

    SDL_RenderPresent(renderer_);
    return true;
}

} // namespace originalengine
