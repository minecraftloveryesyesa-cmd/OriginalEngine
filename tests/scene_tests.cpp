#include "engine/Scene.hpp"

#include <iostream>

int main() {
    originalengine::Scene scene;
    originalengine::Transform transform;
    transform.position = {1.0F, 2.0F, 3.0F};
    transform.scale = {2.0F, 2.0F, 2.0F};

    if (!scene.addObject("Player", transform)) {
        std::cerr << "Adding a scene object failed.\n";
        return 1;
    }
    if (scene.addObject("Player")) {
        std::cerr << "Duplicate object names should be rejected.\n";
        return 1;
    }
    if (scene.addObject("")) {
        std::cerr << "Empty object names should be rejected.\n";
        return 1;
    }

    const originalengine::SceneObject* player = scene.findObject("Player");
    if (player == nullptr || player->transform.position.x != 1.0F ||
        player->transform.position.y != 2.0F ||
        player->transform.position.z != 3.0F ||
        player->transform.scale.x != 2.0F) {
        std::cerr << "Scene object transform was not preserved.\n";
        return 1;
    }

    if (!player->visible) {
        std::cerr << "Scene objects should be visible by default.\n";
        return 1;
    }
    player->visible = false;
    if (player->visible) {
        std::cerr << "Scene visibility flag did not retain its value.\n";
        return 1;
    }

    if (scene.objectCount() != 1 || !scene.removeObject("Player") ||
        scene.removeObject("Player") || scene.findObject("Player") != nullptr ||
        scene.objectCount() != 0) {
        std::cerr << "Removing a scene object failed.\n";
        return 1;
    }

    std::cout << "Scene tests passed.\n";
    return 0;
}
