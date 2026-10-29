#include "engine/Renderer3D.hpp"

#include <iostream>

int main() {
    const SDL_Point invalid = originalengine::Renderer3D::softwareRenderSize(0, 720);
    if (invalid.x != 0 || invalid.y != 0) {
        std::cerr << "Invalid output sizes must remain invalid.\n";
        return 1;
    }

    const SDL_Point native = originalengine::Renderer3D::softwareRenderSize(800, 450);
    if (native.x != 800 || native.y != 450) {
        std::cerr << "Sizes within the software budget must not be scaled.\n";
        return 1;
    }

    const SDL_Point scaled = originalengine::Renderer3D::softwareRenderSize(1920, 1080);
    if (scaled.x != 800 || scaled.y != 450) {
        std::cerr << "Large output must be scaled to the software pixel budget.\n";
        return 1;
    }

    const SDL_Point portrait = originalengine::Renderer3D::softwareRenderSize(1080, 1920);
    if (static_cast<std::size_t>(portrait.x) * static_cast<std::size_t>(portrait.y) >
        800U * 450U) {
        std::cerr << "Scaled output exceeded the software pixel budget.\n";
        return 1;
    }

    std::cout << "Renderer size tests passed.\n";
    return 0;
}
