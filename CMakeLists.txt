cmake_minimum_required(VERSION 3.24)

project(OriginalEngine
    VERSION 0.1.0
    DESCRIPTION "A cross-platform 3D game engine"
    LANGUAGES CXX
)

option(ORIGINALENGINE_BUILD_APP "Build the OriginalEngine sample application" ON)
option(ORIGINALENGINE_USE_SDL3 "Use SDL3 for the application window and input" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(originalengine
    src/engine/Application.cpp
    src/engine/Camera.cpp
    src/engine/CollisionWorld.cpp
    src/engine/Renderer3D.cpp
)

target_include_directories(originalengine
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_compile_features(originalengine PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(originalengine PRIVATE /W4 /permissive-)
else()
    target_compile_options(originalengine PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ORIGINALENGINE_BUILD_APP)
    if(ORIGINALENGINE_USE_SDL3)
        include(FetchContent)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            SDL3
            GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
            GIT_TAG release-3.2.10
            GIT_SHALLOW TRUE
        )
        FetchContent_MakeAvailable(SDL3)
        target_link_libraries(originalengine PUBLIC SDL3::SDL3)

        add_executable(originalengine_app src/main.cpp)
        target_link_libraries(originalengine_app PRIVATE originalengine)
        target_compile_definitions(originalengine_app PRIVATE ORIGINALENGINE_USE_SDL3=1)

        if(WIN32)
            add_custom_command(TARGET originalengine_app POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL3::SDL3>
                    $<TARGET_FILE_DIR:originalengine_app>
            )
        endif()
    else()
        message(FATAL_ERROR "The sample app currently requires SDL3. Set ORIGINALENGINE_BUILD_APP=OFF to build the engine library only.")
    endif()
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(originalengine_camera_tests tests/camera_tests.cpp)
    target_link_libraries(originalengine_camera_tests PRIVATE originalengine)
    add_test(NAME camera_controls COMMAND originalengine_camera_tests)\n\n    add_executable(originalengine_collision_tests tests/collision_tests.cpp)\n    target_link_libraries(originalengine_collision_tests PRIVATE originalengine)\n    add_test(NAME collision_world COMMAND originalengine_collision_tests)
endif()
