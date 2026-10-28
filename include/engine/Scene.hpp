#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace originalengine {

struct Vec3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct Transform {
    Vec3 position{};
    Vec3 rotation{};
    Vec3 scale{1.0F, 1.0F, 1.0F};
};

struct SceneObject {
    std::string name;
    Transform transform{};
    bool visible{true};
};

// A small scene registry. Object names are unique within each scene.
class Scene {
public:
    [[nodiscard]] bool addObject(std::string name, Transform transform = {});
    [[nodiscard]] bool removeObject(const std::string& name) noexcept;

    [[nodiscard]] SceneObject* findObject(const std::string& name) noexcept;
    [[nodiscard]] const SceneObject* findObject(const std::string& name) const noexcept;

    [[nodiscard]] const std::vector<SceneObject>& objects() const noexcept {
        return objects_;
    }

    [[nodiscard]] std::size_t objectCount() const noexcept {
        return objects_.size();
    }

private:
    std::vector<SceneObject> objects_;
};

} // namespace originalengine
