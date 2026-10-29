#pragma once

#include <SDL3/SDL.h>

#include <vector>

#include "engine/Scene.hpp"

namespace originalengine {

class Camera;
struct Mesh;

struct MeshInstance {
    const Mesh* mesh{nullptr};
    const SceneObject* object{nullptr};
};

// A perspective-projected 3D renderer with a movable first-person camera.
class Renderer3D {
public:
    explicit Renderer3D(SDL_Renderer* renderer);
    ~Renderer3D();

    Renderer3D(const Renderer3D&) = delete;
    Renderer3D& operator=(const Renderer3D&) = delete;

    [[nodiscard]] bool render(int width, int height, double elapsedSeconds,
                              const Camera& camera,
                              const std::vector<MeshInstance>& meshInstances = {},
                              bool wireframe = false);

private:
    SDL_Renderer* renderer_;
    SDL_Texture* depthTexture_{nullptr};
    int textureWidth_{0};
    int textureHeight_{0};
    std::vector<Uint8> colorBuffer_;
    std::vector<float> depthBuffer_;
    double previousElapsedSeconds_{0.0};
    float smoothedFramesPerSecond_{0.0F};
};

} // namespace originalengine
