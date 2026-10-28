#include "engine/Application.hpp"

#include <SDL3/SDL.h>

#include <iostream>

namespace {

void logSdlError(const char* operation) {
    std::cerr << "[OriginalEngine] " << operation << " failed: "
              << SDL_GetError() << '\n';
}

} // namespace

int main() {
    using originalengine::Application;
    using originalengine::ApplicationConfig;

    const ApplicationConfig config{
        .title = "OriginalEngine",
        .width = 1280,
        .height = 720,
        .resizable = true,
    };

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        logSdlError("SDL_Init");
        return 1;
    }

    const SDL_WindowFlags flags = config.resizable ? SDL_WINDOW_RESIZABLE : 0;
    SDL_Window* window = SDL_CreateWindow(
        config.title.c_str(),
        config.width,
        config.height,
        flags
    );

    if (window == nullptr) {
        logSdlError("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    bool running = true;
    while (running) {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN &&
                       event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Rendering will be added in the next milestone.
        SDL_Delay(1);
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
