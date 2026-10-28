#include "engine/Camera.hpp"

#include <cmath>
#include <iostream>

namespace {
bool nearlyEqual(float a, float b, float epsilon = 0.0001F) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    originalengine::Camera camera;

    camera.move(1.0F, 0.0F, 0.0F, 1.0F);
    if (!nearlyEqual(camera.z(), 4.0F) || !nearlyEqual(camera.x(), 0.0F)) {
        std::cerr << "Camera forward movement failed.\n";
        return 1;
    }

    camera.move(0.0F, 1.0F, 0.0F, 0.5F);
    if (!nearlyEqual(camera.x(), 2.0F)) {
        std::cerr << "Camera strafe movement failed.\n";
        return 1;
    }

    const float oldYaw = camera.yaw();
    const float oldPitch = camera.pitch();
    camera.rotate(100.0F, -40.0F);
    if (!(camera.yaw() > oldYaw) || !(camera.pitch() > oldPitch)) {
        std::cerr << "Camera mouse rotation failed.\n";
        return 1;
    }

    camera.rotate(0.0F, 100000.0F);
    if (std::fabs(camera.pitch()) >= 1.5707963F) {
        std::cerr << "Camera pitch clamp failed.\n";
        return 1;
    }

    const float oldX = camera.x();
    camera.move(1.0F, 1.0F, 0.0F, 1.0F, true);
    if (nearlyEqual(camera.x(), oldX)) {
        std::cerr << "Camera diagonal/fast movement failed.\n";
        return 1;
    }

    std::cout << "Camera tests passed.\n";
    return 0;
}
