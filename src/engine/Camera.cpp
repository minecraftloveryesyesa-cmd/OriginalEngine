#include "engine/Camera.hpp"
#include "engine/CollisionWorld.hpp"

#include <algorithm>
#include <cmath>

namespace originalengine {
namespace {
constexpr float kMinimumMouseSensitivity = 0.0005F;
constexpr float kMaximumMouseSensitivity = 0.01F;
constexpr float kPi = 3.14159265358979323846F;
constexpr float kWalkSpeed = 4.0F;
constexpr float kFastSpeed = 10.0F;
}

void Camera::reset() noexcept {
    x_ = 0.0F;
    y_ = 0.0F;
    z_ = 0.0F;
    yaw_ = 0.0F;
    pitch_ = 0.0F;
}

void Camera::adjustMouseSensitivity(float delta) noexcept {
    mouseSensitivity_ = std::clamp(mouseSensitivity_ + delta,
                                   kMinimumMouseSensitivity,
                                   kMaximumMouseSensitivity);
}

void Camera::rotate(float mouseDeltaX, float mouseDeltaY) {
    yaw_ += mouseDeltaX * mouseSensitivity_;
    pitch_ -= mouseDeltaY * mouseSensitivity_;
    pitch_ = std::clamp(pitch_, -kPi * 0.49F, kPi * 0.49F);
}

void Camera::move(float forwardAxis, float rightAxis, float verticalAxis,
                  float deltaSeconds, bool fast,
                  const CollisionWorld* collisionWorld, float collisionRadius) {
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

    const float dx = (sinYaw * forwardAxis + cosYaw * rightAxis) * speed * deltaSeconds;
    const float dz = (cosYaw * forwardAxis - sinYaw * rightAxis) * speed * deltaSeconds;
    const float dy = verticalAxis * speed * deltaSeconds;

    if (collisionWorld == nullptr) {
        x_ += dx;
        y_ += dy;
        z_ += dz;
        return;
    }

    // Resolve each axis separately to prevent wall penetration while allowing sliding.
    x_ += collisionWorld->resolveAxis(x_, y_, z_, dx, collisionRadius, 0);
    y_ += collisionWorld->resolveAxis(x_, y_, z_, dy, collisionRadius, 1);
    z_ += collisionWorld->resolveAxis(x_, y_, z_, dz, collisionRadius, 2);
}

} // namespace originalengine
