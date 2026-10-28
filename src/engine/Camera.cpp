#include "engine/Camera.hpp"\n#include "engine/CollisionWorld.hpp"

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
                  float deltaSeconds, bool fast,\n                  const CollisionWorld* collisionWorld, float collisionRadius) {
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
