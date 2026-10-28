#include "engine/Camera.hpp"

#include <algorithm>
#include <cmath>

namespace originalengine {
namespace {
constexpr float kMouseSensitivity = 0.0025F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kWalkSpeed = 4.0F;
constexpr float kFastSpeed = 10.0F;
}

void Camera::rotate(float mouseDeltaX, float mouseDeltaY) {
    yaw_ += mouseDeltaX * kMouseSensitivity;
    pitch_ -= mouseDeltaY * kMouseSensitivity;
    pitch_ = std::clamp(pitch_, -kPi * 0.49F, kPi * 0.49F);
}

void Camera::move(float forwardAxis, float rightAxis, float verticalAxis,
                  float deltaSeconds, bool fast) {
    if (deltaSeconds <= 0.0F) {
        return;
    }

    forwardAxis = std::clamp(forwardAxis, -1.0F, 1.0F);
    rightAxis = std::clamp(rightAxis, -1.0F, 1.0F);
    verticalAxis = std::clamp(verticalAxis, -1.0F, 1.0F);

    const float length = std::sqrt(forwardAxis * forwardAxis +
                                   rightAxis * rightAxis +
                                   verticalAxis * verticalAxis);
    if (length > 1.0F) {
        forwardAxis /= length;
        rightAxis /= length;
        verticalAxis /= length;
    }

    const float speed = fast ? kFastSpeed : kWalkSpeed;
    const float sinYaw = std::sin(yaw_);
    const float cosYaw = std::cos(yaw_);

    // Movement stays level with the ground; looking up does not make WASD fly.
    x_ += (sinYaw * forwardAxis + cosYaw * rightAxis) * speed * deltaSeconds;
    z_ += (cosYaw * forwardAxis - sinYaw * rightAxis) * speed * deltaSeconds;
    y_ += verticalAxis * speed * deltaSeconds;
}

} // namespace originalengine
