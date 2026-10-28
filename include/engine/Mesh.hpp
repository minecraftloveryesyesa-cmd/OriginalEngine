#pragma once

#include <array>
#include <cstddef>
#include <vector>

namespace originalengine {

struct MeshVertex {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

struct MeshTriangle {
    std::array<std::size_t, 3> indices{};
};

// Indexed triangle mesh. Faces reference entries in vertices.
struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<MeshTriangle> triangles;

    [[nodiscard]] bool empty() const noexcept {
        return vertices.empty() || triangles.empty();
    }
};

} // namespace originalengine
