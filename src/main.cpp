#include "engine/Renderer3D.hpp"

#include <SDL3/SDL.h>

#include <exception>
#include <iostream>
#include <string_view>

namespace {

void logSdlError(const char* operation) {
    std::cerr << "[OriginalEngine] " << operation << " failed: "
              << SDL_GetError() << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    const bool smokeTest = argc > 1 && std::string_view(argv[1]) == "--smoke-test";
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        logSdlError("SDL_Init");
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        smokeTest ? "OriginalEngine - CI Smoke Test" : "OriginalEngine - 3D Renderer",
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE
    );

    if (window == nullptr) {
        logSdlError("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        logSdlError("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    if (!smokeTest) {
        SDL_SetRenderVSync(renderer, 1);
    }

    int exitCode = 0;
    try {
        originalengine::Renderer3D renderer3D(renderer);
        bool running = true;
        const Uint64 startTicks = SDL_GetTicks();

        while (running) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN &&
                           event.key.key == SDLK_ESCAPE &&
                           !event.key.repeat) {
                    running = false;
                }
            }

            if (!running) {
                break;
            }

            int width = 0;
            int height = 0;
            SDL_GetWindowSize(window, &width, &height);

            const double elapsedSeconds =
                static_cast<double>(SDL_GetTicks() - startTicks) / 1000.0;
            if (!renderer3D.render(width, height, elapsedSeconds)) {
                std::cerr << "[OriginalEngine] Rendering a frame failed.\n";
                exitCode = 1;
                break;
            }

            if (smokeTest) {
                std::cout << "[OriginalEngine] Smoke test passed: SDL initialized, "
                             "window and renderer created, and one frame rendered.\n";
                running = false;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "[OriginalEngine] " << error.what() << '\n';
        exitCode = 1;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exitCode;
}
