#pragma once

namespace originalengine {\n\nclass CollisionWorld;

// First-person camera using +Z as the initial forward direction.
class Camera {
public:
    Camera() = default;

    void rotate(float mouseDeltaX, float mouseDeltaY);
    void move(float forwardAxis, float rightAxis, float verticalAxis,
              float deltaSeconds, bool fast = false,\n              const CollisionWorld* collisionWorld = nullptr,\n              float collisionRadius = 0.25F);

    [[nodiscard]] float x() const noexcept { return x_; }
    [[nodiscard]] float y() const noexcept { return y_; }
    [[nodiscard]] float z() const noexcept { return z_; }
    [[nodiscard]] float yaw() const noexcept { return yaw_; }
    [[nodiscard]] float pitch() const noexcept { return pitch_; }

private:
    float x_{0.0F};
    float y_{0.0F};
    float z_{0.0F};
    float yaw_{0.0F};
    float pitch_{0.0F};
};

} // namespace originalengine
