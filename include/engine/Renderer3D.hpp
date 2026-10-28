#pragma once

#include <SDL3/SDL.h>

namespace originalengine {

// A small software-projected 3D renderer. SDL handles triangle rasterization;
// this class handles 3D transforms, perspective projection, and face ordering.
class Renderer3D {
public:
    explicit Renderer3D(SDL_Renderer* renderer);

    [[nodiscard]] bool render(int width, int height, double elapsedSeconds);

private:
    SDL_Renderer* renderer_;
};

} // namespace originalengine
