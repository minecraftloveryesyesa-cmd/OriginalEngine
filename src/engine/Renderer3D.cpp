#include "engine/Renderer3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace originalengine {
namespace {

constexpr float kPi = 3.14159265358979323846F;

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

} // namespace

Renderer3D::Renderer3D(SDL_Renderer* renderer)
    : renderer_(renderer) {
    if (renderer_ == nullptr) {
        throw std::invalid_argument("Renderer3D requires a valid SDL_Renderer.");
    }
}

void Renderer3D::render(int width, int height, double elapsedSeconds) {
    if (width <= 0 || height <= 0) {
        return;
    }

    SDL_SetRenderDrawColor(renderer_, 12, 18, 31, 255);
    SDL_RenderClear(renderer_);

    // A subtle perspective grid makes the depth of the scene easier to read.
    SDL_SetRenderDrawColor(renderer_, 31, 43, 61, 255);
    const float horizon = static_cast<float>(height) * 0.60F;
    for (int i = -10; i <= 10; ++i) {
        const float x = static_cast<float>(width) * 0.5F +
                        static_cast<float>(i) * static_cast<float>(width) * 0.045F;
        SDL_RenderLine(renderer_, static_cast<float>(width) * 0.5F, horizon,
                       x, static_cast<float>(height));
    }
    for (int i = 1; i <= 8; ++i) {
        const float t = static_cast<float>(i) / 8.0F;
        const float y = horizon + (static_cast<float>(height) - horizon) * t * t;
        SDL_RenderLine(renderer_, 0.0F, y, static_cast<float>(width), y);
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
        p.z += 4.2F; // Move the cube in front of the camera.
        transformed[i] = p;
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

    const float scale = static_cast<float>(std::min(width, height)) * 0.42F;
    const float centerX = static_cast<float>(width) * 0.5F;
    const float centerY = static_cast<float>(height) * 0.43F;
    constexpr float focalLength = 2.4F;

    auto project = [&](Vec3 p) -> SDL_FPoint {
        const float perspective = focalLength / p.z;
        return {
            centerX + p.x * scale * perspective,
            centerY - p.y * scale * perspective
        };
    };

    for (const Face& face : faces) {
        std::array<SDL_Vertex, 6> vertices{};
        const std::array<int, 6> triangleIndices{{
            face.indices[0], face.indices[1], face.indices[2],
            face.indices[0], face.indices[2], face.indices[3]
        }};

        for (std::size_t i = 0; i < triangleIndices.size(); ++i) {
            const Vec3 point = transformed[static_cast<std::size_t>(triangleIndices[i])];
            vertices[i].position = project(point);
            vertices[i].color = face.color;
            vertices[i].tex_coord = {0.0F, 0.0F};
        }

        if (!SDL_RenderGeometry(renderer_, nullptr, vertices.data(),
                                static_cast<int>(vertices.size()), nullptr, 0)) {
            SDL_Log("SDL_RenderGeometry failed: %s", SDL_GetError());
            return;
        }
    }

    SDL_RenderPresent(renderer_);
}

} // namespace originalengine
