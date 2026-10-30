#include "engine/Camera.hpp"
#include "engine/CollisionWorld.hpp"
#include "engine/Renderer3D.hpp"
#include "engine/GpuDeviceProbe.hpp"
#include "engine/ObjMeshLoader.hpp"
#include "engine/Scene.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <cmath>
#include <exception>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string diagnosticLogPath;

void writeDiagnostic(const std::string& message) {
    std::cerr << "[OriginalEngine] " << message << '\n';
    if (!diagnosticLogPath.empty()) {
        std::ofstream log(diagnosticLogPath, std::ios::app);
        if (log) log << message << '\n';
    }
}

void initializeDiagnosticLog() {
    char* prefPath = SDL_GetPrefPath("com.gasolineengine", "GasolineEngine");
    if (prefPath != nullptr) {
        diagnosticLogPath = std::string(prefPath) + "startup.log";
        SDL_free(prefPath);
        std::ofstream log(diagnosticLogPath, std::ios::trunc);
        if (log) log << "Gasoline Engine startup diagnostics\n";
    }
}

void logSdlError(const char* operation) {
    writeDiagnostic(std::string(operation) + " failed: " + SDL_GetError());
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Gasoline Engine startup error",
        (std::string(operation) + " failed. Details saved to startup.log.").c_str(), nullptr);
}

bool runGpuClearSmokeTest(SDL_Window* window) {
    constexpr SDL_GPUShaderFormat formats =
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL;
    SDL_GPUDevice* device = SDL_CreateGPUDevice(formats, false, nullptr);
    if (device == nullptr) {
        writeDiagnostic(std::string("SDL_CreateGPUDevice failed: ") + SDL_GetError());
        return false;
    }

    bool success = false;
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        writeDiagnostic(std::string("SDL_ClaimWindowForGPUDevice failed: ") + SDL_GetError());
    } else {
        SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
        if (commandBuffer == nullptr) {
            writeDiagnostic(std::string("SDL_AcquireGPUCommandBuffer failed: ") + SDL_GetError());
        } else {
            SDL_GPUTexture* swapchainTexture = nullptr;
            Uint32 width = 0;
            Uint32 height = 0;
            if (!SDL_WaitAndAcquireGPUSwapchainTexture(
                    commandBuffer, window, &swapchainTexture, &width, &height)) {
                writeDiagnostic(std::string("SDL_WaitAndAcquireGPUSwapchainTexture failed: ") + SDL_GetError());
                (void)SDL_CancelGPUCommandBuffer(commandBuffer);
            } else if (swapchainTexture == nullptr || width == 0 || height == 0) {
                writeDiagnostic("GPU smoke test skipped drawing because the swapchain has no drawable texture");
                if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
                    writeDiagnostic(std::string("SDL_SubmitGPUCommandBuffer failed: ") + SDL_GetError());
                }
            } else {
                SDL_GPUColorTargetInfo colorTarget{};
                colorTarget.texture = swapchainTexture;
                colorTarget.clear_color = SDL_FColor{0.035F, 0.055F, 0.09F, 1.0F};
                colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
                colorTarget.store_op = SDL_GPU_STOREOP_STORE;
                SDL_GPURenderPass* renderPass =
                    SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, nullptr);
                if (renderPass == nullptr) {
                    writeDiagnostic(std::string("SDL_BeginGPURenderPass failed: ") + SDL_GetError());
                    // A swapchain texture has already been acquired, so the
                    // command buffer must be submitted rather than cancelled.
                    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
                        writeDiagnostic(std::string("SDL_SubmitGPUCommandBuffer failed: ") + SDL_GetError());
                    }
                } else {
                    SDL_EndGPURenderPass(renderPass);
                    if (SDL_SubmitGPUCommandBuffer(commandBuffer)) {
                        writeDiagnostic("GPU smoke test passed: swapchain acquired, cleared, and submitted");
                        success = true;
                    } else {
                        writeDiagnostic(std::string("SDL_SubmitGPUCommandBuffer failed: ") + SDL_GetError());
                    }
                }
            }
        }
        SDL_ReleaseWindowFromGPUDevice(device, window);
    }
    SDL_DestroyGPUDevice(device);
    return success;
}

bool initializeGpuBackend(SDL_Window* window, SDL_GPUDevice** outputDevice) {
    constexpr SDL_GPUShaderFormat formats = SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL;
    SDL_GPUDevice* device = SDL_CreateGPUDevice(formats, false, nullptr);
    if (device == nullptr) {
        writeDiagnostic(std::string("SDL_CreateGPUDevice failed: ") + SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        writeDiagnostic(std::string("SDL_ClaimWindowForGPUDevice failed: ") + SDL_GetError());
        SDL_DestroyGPUDevice(device);
        return false;
    }
    *outputDevice = device;
    writeDiagnostic(std::string("GPU preview active: ") + SDL_GetGPUDeviceDriver(device));
    return true;
}

bool renderGpuPreviewFrame(SDL_GPUDevice* device, SDL_Window* window) {
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device);
    if (commandBuffer == nullptr) return false;
    SDL_GPUTexture* texture = nullptr;
    Uint32 width = 0, height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &texture, &width, &height)) {
        (void)SDL_CancelGPUCommandBuffer(commandBuffer);
        return false;
    }
    if (texture == nullptr || width == 0 || height == 0) {
        return SDL_SubmitGPUCommandBuffer(commandBuffer);
    }
    SDL_GPUColorTargetInfo target{};
    target.texture = texture;
    target.clear_color = SDL_FColor{0.025F, 0.12F, 0.19F, 1.0F};
    target.load_op = SDL_GPU_LOADOP_CLEAR;
    target.store_op = SDL_GPU_STOREOP_STORE;
    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commandBuffer, &target, 1, nullptr);
    if (pass == nullptr) return SDL_SubmitGPUCommandBuffer(commandBuffer);
    SDL_EndGPURenderPass(pass);
    return SDL_SubmitGPUCommandBuffer(commandBuffer);
}

void drawTouchCircle(SDL_Renderer* renderer, float cx, float cy, float radius) {
    constexpr int segments = 32;
    for (int i = 0; i < segments; ++i) {
        const float a0 = static_cast<float>(i) * 6.28318530718F / static_cast<float>(segments);
        const float a1 = static_cast<float>(i + 1) * 6.28318530718F / static_cast<float>(segments);
        (void)SDL_RenderLine(renderer, cx + std::cos(a0) * radius,
            cy + std::sin(a0) * radius, cx + std::cos(a1) * radius,
            cy + std::sin(a1) * radius);
    }
}

std::filesystem::path resolveAssetPath(const char* relativePath) {
    const std::filesystem::path relative(relativePath);
    // SDL3 returns a cached path string owned by SDL.
    const char* basePath = SDL_GetBasePath();
    if (basePath != nullptr) {
        const std::filesystem::path executablePath(basePath);
        const std::filesystem::path besideExecutable = executablePath / relative;
        std::error_code error;
        if (std::filesystem::is_regular_file(besideExecutable, error)) {
            return besideExecutable;
        }
    }

    std::error_code error;
    if (std::filesystem::is_regular_file(relative, error)) {
        return std::filesystem::absolute(relative, error);
    }
    std::filesystem::path fallback = relative;
    if (basePath != nullptr) {
        fallback = std::filesystem::path(basePath) / relative;
    }
    return fallback;
}

} // namespace

int main(int argc, char* argv[]) {
    const bool gpuSmokeTest = argc > 1 && std::string_view(argv[1]) == "--gpu-smoke-test";
    const bool smokeTest = argc > 1 && std::string_view(argv[1]) == "--smoke-test";
    initializeDiagnosticLog();
    writeDiagnostic("Stage: entering main");
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    writeDiagnostic("Stage: SDL_Init");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        logSdlError("SDL_Init");
        return 1;
    }

    writeDiagnostic("Stage: SDL_Init succeeded");
    SDL_WindowFlags windowFlags = SDL_WINDOW_RESIZABLE;
#if defined(__APPLE__)
    // The window must be Metal-capable for runtime SDL_GPU activation.
    windowFlags |= SDL_WINDOW_METAL;
#endif

    SDL_Window* window = SDL_CreateWindow(
        smokeTest ? "OriginalEngine - CI Smoke Test" : "OriginalEngine - 3D Camera Demo",
        windowWidth,
        windowHeight,
        windowFlags
    );

    if (window == nullptr) {
        logSdlError("SDL_CreateWindow");
        SDL_Quit();
        return 1;
    }

    writeDiagnostic("Stage: SDL_CreateWindow succeeded");
    if (gpuSmokeTest) {
        const bool passed = runGpuClearSmokeTest(window);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return passed ? 0 : 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (renderer == nullptr) {
        logSdlError("SDL_CreateRenderer");
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    writeDiagnostic("Stage: SDL_CreateRenderer succeeded");
    if (!smokeTest) {
        SDL_SetRenderVSync(renderer, 1);
        SDL_SetWindowTitle(window,
            "Gasoline Engine | WASD move, Q/E up/down, Shift fast | R: reset camera | +/-: mouse sensitivity | Space: pause animation | F11: fullscreen | 1/2: toggle objects | F3: wireframe");
    }

    int exitCode = 0;
    try {
        writeDiagnostic("Stage: creating Renderer3D");
        auto renderer3D = std::make_unique<originalengine::Renderer3D>(renderer);
        SDL_GPUDevice* gpuDevice = nullptr;
        bool gpuMode = false;
        bool backendToggleRequested = false;
        writeDiagnostic("Stage: Renderer3D created");
        originalengine::GpuDeviceProbe gpuProbe;
        std::string gpuError;
        if (gpuProbe.initialize(&gpuError)) {
            writeDiagnostic("SDL_GPU available: driver=" + gpuProbe.driverName() +
                ", shaderFormats=" + std::to_string(
                    static_cast<unsigned int>(gpuProbe.shaderFormats())));
            // This is only a capability probe. Release the unclaimed GPU device
            // immediately so it does not reserve GPU resources while SDL_Renderer
            // continues to own and present the window.
            gpuProbe.shutdown();
        } else {
            writeDiagnostic("SDL_GPU unavailable; CPU renderer remains active: " + gpuError);
        }
        originalengine::Camera camera;
        originalengine::CollisionWorld collisionWorld;
        originalengine::Mesh demoMesh;
        std::string meshError;
        const std::filesystem::path meshPath = resolveAssetPath("assets/demo.obj");
        const std::filesystem::path texturePath = resolveAssetPath("assets/demo_texture.ppm");
        writeDiagnostic("Stage: loading OBJ: " + meshPath.string());
        if (!originalengine::ObjMeshLoader::loadFromFile(meshPath.string(), demoMesh, &meshError)) {
            writeDiagnostic("Could not load mesh " + meshPath.string() + ": " + meshError);
            exitCode = 1;
        }
        writeDiagnostic("Stage: OBJ load completed");
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
            {&demoMesh, demoObject, texturePath.string()},
            {&demoMesh, leftObject, texturePath.string()}
        };
        // Conservative static bounds around the demo cube.
        collisionWorld.addObstacle({-1.4F, -1.4F, 2.8F, 1.4F, 1.4F, 5.6F});
        bool running = true;
        bool mouseCaptured = false;
        bool wireframe = false;
        bool animationPaused = false;
        bool fullscreen = false;
        SDL_FingerID movementFinger = 0;
        SDL_FingerID lookFinger = 0;
        bool hasMovementFinger = false;
        bool hasLookFinger = false;
        float movementX = 0.0F;
        float movementY = 0.0F;
        float lookDeltaX = 0.0F;
        float lookDeltaY = 0.0F;
        Uint64 previousTicks = SDL_GetTicks();
        double elapsedSeconds = 0.0;

        writeDiagnostic("Stage: entering main loop");
        while (running && exitCode == 0) {
            int touchWidth = 0;
            int touchHeight = 0;
            (void)SDL_GetRenderOutputSize(renderer, &touchWidth, &touchHeight);
            SDL_Event event{};
            while (SDL_PollEvent(&event)) {
                if (!smokeTest && touchWidth > 0 && touchHeight > 0 &&
                    (event.type == SDL_EVENT_FINGER_DOWN || event.type == SDL_EVENT_FINGER_MOTION ||
                     event.type == SDL_EVENT_FINGER_UP)) {
                    if (event.type == SDL_EVENT_FINGER_DOWN) {
                        if (event.tfinger.x >= 0.78F && event.tfinger.y <= 0.16F) {
                            backendToggleRequested = true;
                        } else if (event.tfinger.x < 0.5F && !hasMovementFinger) {
                            movementFinger = event.tfinger.fingerID;
                            hasMovementFinger = true;
                            movementX = event.tfinger.x;
                            movementY = event.tfinger.y;
                        } else if (event.tfinger.x >= 0.5F && !hasLookFinger) {
                            lookFinger = event.tfinger.fingerID;
                            hasLookFinger = true;
                        }
                    } else if (event.type == SDL_EVENT_FINGER_MOTION) {
                        if (hasMovementFinger && event.tfinger.fingerID == movementFinger) {
                            movementX = event.tfinger.x;
                            movementY = event.tfinger.y;
                        } else if (hasLookFinger && event.tfinger.fingerID == lookFinger) {
                            lookDeltaX += event.tfinger.dx * static_cast<float>(touchWidth);
                            lookDeltaY += event.tfinger.dy * static_cast<float>(touchHeight);
                        }
                    } else {
                        if (hasMovementFinger && event.tfinger.fingerID == movementFinger) {
                            hasMovementFinger = false;
                            movementX = 0.0F;
                            movementY = 0.0F;
                        }
                        if (hasLookFinger && event.tfinger.fingerID == lookFinger) {
                            hasLookFinger = false;
                        }
                    }
                } else if (event.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_G) {
                    backendToggleRequested = true;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_R) {
                    camera.reset();
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_MINUS) {
                    camera.adjustMouseSensitivity(-0.0005F);
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_EQUALS) {
                    camera.adjustMouseSensitivity(0.0005F);
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_SPACE) {
                    animationPaused = !animationPaused;
                } else if (!smokeTest && event.type == SDL_EVENT_KEY_DOWN &&
                           !event.key.repeat && event.key.key == SDLK_F11) {
                    const bool requestedFullscreen = !fullscreen;
                    if (SDL_SetWindowFullscreen(window, requestedFullscreen)) {
                        fullscreen = requestedFullscreen;
                    } else {
                        logSdlError("SDL_SetWindowFullscreen");
                    }
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

            if (backendToggleRequested) {
                backendToggleRequested = false;
                if (!gpuMode) {
                    renderer3D.reset();
                    SDL_DestroyRenderer(renderer);
                    renderer = nullptr;
                    if (initializeGpuBackend(window, &gpuDevice)) {
                        gpuMode = true;
                        SDL_SetWindowTitle(window, "Gasoline Engine | GPU preview | G: switch to CPU");
                    } else {
                        renderer = SDL_CreateRenderer(window, nullptr);
                        if (renderer != nullptr) {
                            renderer3D = std::make_unique<originalengine::Renderer3D>(renderer);
                            writeDiagnostic("GPU activation failed; CPU renderer restored");
                        } else {
                            logSdlError("SDL_CreateRenderer after GPU activation failure");
                            exitCode = 1;
                        }
                    }
                } else {
                    SDL_ReleaseWindowFromGPUDevice(gpuDevice, window);
                    SDL_DestroyGPUDevice(gpuDevice);
                    gpuDevice = nullptr;
                    gpuMode = false;
                    renderer = SDL_CreateRenderer(window, nullptr);
                    if (renderer != nullptr) {
                        (void)SDL_SetRenderVSync(renderer, 1);
                        renderer3D = std::make_unique<originalengine::Renderer3D>(renderer);
                        SDL_SetWindowTitle(window, "Gasoline Engine | G: switch to GPU");
                        writeDiagnostic("CPU renderer restored");
                    } else {
                        logSdlError("SDL_CreateRenderer after GPU mode");
                        exitCode = 1;
                    }
                }
            }
            if (exitCode != 0) break;

            const Uint64 currentTicks = SDL_GetTicks();
            const float deltaSeconds = std::min(
                static_cast<float>(currentTicks - previousTicks) / 1000.0F, 0.1F);
            previousTicks = currentTicks;
            if (!animationPaused) {
                elapsedSeconds += static_cast<double>(deltaSeconds);
            }

            if (!smokeTest) {
                const bool* keys = SDL_GetKeyboardState(nullptr);
                float forward = static_cast<float>(keys[SDL_SCANCODE_W]) -
                                      static_cast<float>(keys[SDL_SCANCODE_S]);
                float right = static_cast<float>(keys[SDL_SCANCODE_D]) -
                                    static_cast<float>(keys[SDL_SCANCODE_A]);
                const float vertical = static_cast<float>(keys[SDL_SCANCODE_E]) -
                                       static_cast<float>(keys[SDL_SCANCODE_Q]);
                const bool fast = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
                if (hasMovementFinger && touchWidth > 0 && touchHeight > 0) {
                    const float anchorX = static_cast<float>(touchWidth) * 0.20F;
                    const float anchorY = static_cast<float>(touchHeight) * 0.78F;
                    const float radius = static_cast<float>(std::min(touchWidth, touchHeight)) * 0.14F;
                    right = std::clamp((movementX * static_cast<float>(touchWidth) - anchorX) / radius, -1.0F, 1.0F);
                    forward = std::clamp(-(movementY * static_cast<float>(touchHeight) - anchorY) / radius, -1.0F, 1.0F);
                }
                camera.rotate(lookDeltaX, lookDeltaY);
                lookDeltaX = 0.0F;
                lookDeltaY = 0.0F;
                camera.move(forward, right, vertical, deltaSeconds, fast, &collisionWorld);
            }

            if (gpuMode) {
                if (!renderGpuPreviewFrame(gpuDevice, window)) {
                    writeDiagnostic(std::string("GPU preview frame failed: ") + SDL_GetError());
                    exitCode = 1;
                    break;
                }
                continue;
            }

            int width = 0;
            int height = 0;
            if (!SDL_GetRenderOutputSize(renderer, &width, &height)) {
                logSdlError("SDL_GetRenderOutputSize");
                exitCode = 1;
                break;
            }

            if (!renderer3D->render(width, height, elapsedSeconds, camera,
                                  meshInstances, wireframe)) {
                writeDiagnostic("Rendering a frame failed");
                exitCode = 1;
                break;
            }

            if (!smokeTest) {
                const float minDimension = static_cast<float>(std::min(width, height));
                const float joyX = static_cast<float>(width) * 0.20F;
                const float joyY = static_cast<float>(height) * 0.78F;
                const float joyRadius = minDimension * 0.14F;
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(renderer, 210, 225, 245, 110);
                drawTouchCircle(renderer, joyX, joyY, joyRadius);
                drawTouchCircle(renderer, joyX, joyY, joyRadius * 0.38F);
                if (hasMovementFinger) {
                    const float knobX = joyX + std::clamp(movementX * static_cast<float>(width) - joyX, -joyRadius, joyRadius);
                    const float knobY = joyY + std::clamp(movementY * static_cast<float>(height) - joyY, -joyRadius, joyRadius);
                    SDL_SetRenderDrawColor(renderer, 120, 190, 255, 180);
                    drawTouchCircle(renderer, knobX, knobY, joyRadius * 0.25F);
                }
                SDL_SetRenderDrawColor(renderer, 210, 225, 245, 90);
                drawTouchCircle(renderer, static_cast<float>(width) * 0.82F,
                                static_cast<float>(height) * 0.78F, minDimension * 0.12F);
            }

            // Present exactly once per frame, after the 2D touch controls.
            (void)SDL_RenderPresent(renderer);

            if (smokeTest) {
                writeDiagnostic("Smoke test passed: one frame rendered");
                running = false;
            }
        }
    } catch (const std::exception& error) {
        writeDiagnostic(std::string("Exception: ") + error.what());
        exitCode = 1;
    }

    writeDiagnostic(std::string("Stage: shutting down with exit code ") + std::to_string(exitCode));
    renderer3D.reset();
    if (gpuDevice != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(gpuDevice, window);
        SDL_DestroyGPUDevice(gpuDevice);
    }
    if (renderer != nullptr) SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return exitCode;
}
