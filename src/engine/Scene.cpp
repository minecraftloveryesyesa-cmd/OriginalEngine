#include "engine/Scene.hpp"

#include <algorithm>
#include <utility>

namespace originalengine {

bool Scene::addObject(std::string name, Transform transform) {
    if (name.empty() || findObject(name) != nullptr) {
        return false;
    }

    objects_.push_back(SceneObject{std::move(name), transform, true});
    return true;
}

bool Scene::removeObject(const std::string& name) noexcept {
    const auto it = std::find_if(objects_.begin(), objects_.end(),
        [&name](const SceneObject& object) { return object.name == name; });
    if (it == objects_.end()) {
        return false;
    }

    objects_.erase(it);
    return true;
}

SceneObject* Scene::findObject(const std::string& name) noexcept {
    const auto it = std::find_if(objects_.begin(), objects_.end(),
        [&name](const SceneObject& object) { return object.name == name; });
    return it == objects_.end() ? nullptr : &(*it);
}

const SceneObject* Scene::findObject(const std::string& name) const noexcept {
    const auto it = std::find_if(objects_.begin(), objects_.end(),
        [&name](const SceneObject& object) { return object.name == name; });
    return it == objects_.end() ? nullptr : &(*it);
}

} // namespace originalengine
