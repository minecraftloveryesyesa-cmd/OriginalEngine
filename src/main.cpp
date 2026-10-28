#include "engine/Camera.hpp"
#include "engine/CollisionWorld.hpp"
#include "engine/Renderer3D.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
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
        smokeTest ? "OriginalEngine - CI Smoke Test" : "OriginalEngine - 3D Camera Demo",
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
        SDL_SetWindowTitle(window,
            "OriginalEngine | WASD move, Q/E up/down, Shift fast | Right-click: mouse look | Esc: exit");
    }

    int exitCode = 0;
    try {
        originalengine::Renderer3D renderer3D(renderer);
        originalengine::Camera camera;
        originalengine::CollisionWorld collisionWorld;
        // Conservative static bounds around the demo cube.
        collisionWorld.addObstacle({-1.4F, -1.4F, 2.8F, 1.4F, 1.4F, 5.6F});
        bool running = true;
        bool mouseCaptured = false;
        Uint64 previousTicks = SDL_GetTicks();
        double elapsedSeconds = 0.0;

        while (running) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (event.type == SDL_EVENT_KEY_DOWN &&
                           event.key.key == SDLK_ESCAPE && !event.key.repeat) {
                    if (mouseCaptured) {
                        SDL_SetWindowRelativeMouseMode(window, false);
                        mouseCaptured = false;
                    } else {
                        running = false;
                    }
                } else if (!smokeTest &&
                           event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
                           event.button.button == SDL_BUTTON_RIGHT) {
                    mouseCaptured = !mouseCaptured;
                    if (!SDL_SetWindowRelativeMouseMode(window, mouseCaptured)) {
                        logSdlError("SDL_SetWindowRelativeMouseMode");
                        mouseCaptured = false;
                    }
                } else if (!smokeTest && mouseCaptured &&
                           event.type == SDL_EVENT_MOUSE_MOTION) {
                    camera.rotate(static_cast<float>(event.motion.xrel),
                                  static_cast<float>(event.motion.yrel));
                }
            }

            if (!running) {
                break;
            }

            const Uint64 currentTicks = SDL_GetTicks();
            const float deltaSeconds = std::min(
                static_cast<float>(currentTicks - previousTicks) / 1000.0F, 0.1F);
            previousTicks = currentTicks;
            elapsedSeconds += static_cast<double>(deltaSeconds);

            if (!smokeTest) {
                const bool* keys = SDL_GetKeyboardState(nullptr);
                const float forward = static_cast<float>(keys[SDL_SCANCODE_W]) -
                                      static_cast<float>(keys[SDL_SCANCODE_S]);
                const float right = static_cast<float>(keys[SDL_SCANCODE_D]) -
                                    static_cast<float>(keys[SDL_SCANCODE_A]);
                const float vertical = static_cast<float>(keys[SDL_SCANCODE_E]) -
                                       static_cast<float>(keys[SDL_SCANCODE_Q]);
                const bool fast = keys[SDL_SCANCODE_LSHIFT] ||
                                  keys[SDL_SCANCODE_RSHIFT];
                camera.move(forward, right, vertical, deltaSeconds, fast, &collisionWorld);
            }

            int width = 0;
            int height = 0;
            SDL_GetWindowSize(window, &width, &height);

            if (!renderer3D.render(width, height, elapsedSeconds, camera)) {
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
