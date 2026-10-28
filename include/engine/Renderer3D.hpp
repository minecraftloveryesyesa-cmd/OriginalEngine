#pragma once

#include <SDL3/SDL.h>

#include "engine/Scene.hpp"

namespace originalengine {

class Camera;
struct Mesh;

// A perspective-projected 3D renderer with a movable first-person camera.
class Renderer3D {
public:
    explicit Renderer3D(SDL_Renderer* renderer);

    [[nodiscard]] bool render(int width, int height, double elapsedSeconds,
                              const Camera& camera, const Mesh* mesh = nullptr,
                              const Transform* meshTransform = nullptr);

private:
    SDL_Renderer* renderer_;
};

} // namespace originalengine
