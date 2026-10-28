#include "engine/CollisionWorld.hpp"

#include <cmath>
#include <iostream>

namespace {
bool nearlyEqual(float a, float b, float epsilon = 0.0001F) {
    return std::fabs(a - b) <= epsilon;
}
}

int main() {
    originalengine::CollisionWorld world;
    world.addObstacle({-1.0F, -1.0F, 3.0F, 1.0F, 1.0F, 5.0F});

    if (!world.overlaps(0.0F, 0.0F, 4.0F, 0.25F)) {
        std::cerr << "Obstacle overlap was not detected.\n";
        return 1;
    }
    if (world.overlaps(0.0F, 0.0F, 2.0F, 0.25F)) {
        std::cerr << "A non-overlapping camera was reported as colliding.\n";
        return 1;
    }

    const float blocked = world.resolveAxis(0.0F, 0.0F, 2.0F, 1.0F, 0.25F, 2);
    if (!nearlyEqual(blocked, 0.0F)) {
        std::cerr << "Movement into a wall was not blocked.\n";
        return 1;
    }

    const float slide = world.resolveAxis(0.0F, 0.0F, 2.0F, 1.0F, 0.25F, 0);
    if (!nearlyEqual(slide, 1.0F)) {
        std::cerr << "Movement parallel to a wall should be allowed.\n";
        return 1;
    }

    std::cout << "Collision tests passed.\n";
    return 0;
}
