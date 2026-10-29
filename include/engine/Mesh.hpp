#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace originalengine {

struct MeshVertex {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
    float u{0.0F};
    float v{0.0F};
    float nx{0.0F};
    float ny{0.0F};
    float nz{0.0F};
    bool hasTexCoord{false};
    bool hasNormal{false};
};

struct MeshTriangle {
    std::array<std::size_t, 3> indices{};
};

// Indexed triangle mesh. OBJ vertices may be duplicated when a position has
// different UV or normal indices on different face corners.
struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> triangles;

    [[nodiscard]] bool empty() const noexcept {
        return vertices.empty() || triangles.empty();
    }
};

} // namespace originalengine
