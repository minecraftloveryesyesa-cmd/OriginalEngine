#include "engine/CollisionWorld.hpp"

#include <algorithm>

namespace originalengine {

void CollisionWorld::addObstacle(Aabb obstacle) {
    if (obstacle.minX > obstacle.maxX || obstacle.minY > obstacle.maxY ||
        obstacle.minZ > obstacle.maxZ) {
        return;
    }
    obstacles_.push_back(obstacle);
}

bool CollisionWorld::overlaps(float x, float y, float z, float radius) const noexcept {
    radius = std::max(radius, 0.0F);
    for (const Aabb& box : obstacles_) {
        if (x + radius > box.minX && x - radius < box.maxX &&
            y + radius > box.minY && y - radius < box.maxY &&
            z + radius > box.minZ && z - radius < box.maxZ) {
            return true;
        }
    }
    return false;
}

float CollisionWorld::resolveAxis(float currentX, float currentY, float currentZ,
                                  float displacement, float radius,
                                  int axis) const noexcept {
    float candidateX = currentX;
    float candidateY = currentY;
    float candidateZ = currentZ;
    if (axis == 0) {
        candidateX += displacement;
    } else if (axis == 1) {
        candidateY += displacement;
    } else if (axis == 2) {
        candidateZ += displacement;
    } else {
        return 0.0F;
    }

    if (overlaps(candidateX, candidateY, candidateZ, radius)) {
        return 0.0F;
    }
    return displacement;
}

} // namespace originalengine
