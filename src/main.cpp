#include "engine/Camera.hpp"
#include "engine/CollisionWorld.hpp"
#include "engine/Renderer3D.hpp"
#include "engine/ObjMeshLoader.hpp"
#include "engine/Scene.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

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
            "OriginalEngine | WASD move, Q/E up/down, Shift fast | 1/2: toggle pyramids | Right-click: mouse look | Esc: exit");
    }

    int exitCode = 0;
    try {
        originalengine::Renderer3D renderer3D(renderer);
        originalengine::Camera camera;
        originalengine::CollisionWorld collisionWorld;
        originalengine::Mesh demoMesh;
        std::string meshError;
        if (!originalengine::ObjMeshLoader::loadFromFile("assets/demo.obj", demoMesh, &meshError)) {
            std::cerr << "[OriginalEngine] Could not load assets/demo.obj: "
                      << meshError << '\n';
            exitCode = 1;
        }
        originalengine::Scene scene;
        originalengine::Transform demoTransform;
        demoTransform.position = {2.6F, -0.2F, 7.0F};
        demoTransform.rotation = {0.0F, 0.25F, 0.0F};
        demoTransform.scale = {0.9F, 0.9F, 0.9F};
        if (!scene.addObject("DemoPyramid", demoTransform)) {
            std::cerr << "[OriginalEngine] Could not create demo scene object.\n";
            exitCode = 1;
        }
        originalengine::Transform secondTransform = demoTransform;
        secondTransform.position.x = -2.6F;
        secondTransform.rotation.y = -0.35F;
        if (!scene.addObject("DemoPyramidLeft", secondTransform)) {
            std::cerr << "[OriginalEngine] Could not create the second demo scene object.\n";
            exitCode = 1;
        }

        // Resolve object pointers after all additions so vector reallocation cannot invalidate them.
        originalengine::SceneObject* demoObject = scene.findObject("DemoPyramid");
        originalengine::SceneObject* leftObject = scene.findObject("DemoPyramidLeft");
        if (demoObject == nullptr || leftObject == nullptr) {
            std::cerr << "[OriginalEngine] A demo scene object was not found.\n";
            exitCode = 1;
        }
        const std::vector<originalengine::MeshInstance> meshInstances{
            {&demoMesh, demoObject},
            {&demoMesh, leftObject}
        };
        // Conservative static bounds around the demo cube.
        collisionWorld.addObstacle({-1.4F, -1.4F, 2.8F, 1.4F, 1.4F, 5.6F});
        bool running = true;
        bool mouseCaptured = false;
        bool wireframe = false;
        Uint64 previousTicks = SDL_GetTicks();
        double elapsedSeconds = 0.0;

        while (running && exitCode == 0) {
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_F3) {
                    wireframe = !wireframe;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_1 &&
                           demoObject != nullptr) {
                    demoObject->visible = !demoObject->visible;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_2 &&
                           leftObject != nullptr) {
                    leftObject->visible = !leftObject->visible;
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

            if (!renderer3D.render(width, height, elapsedSeconds, camera,
                                  meshInstances, wireframe)) {
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
