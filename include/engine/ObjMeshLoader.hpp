#pragma once

#include "engine/Mesh.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>

namespace originalengine {

// Loads Wavefront OBJ positions, optional texture coordinates/normals, and faces.
// Polygon faces are triangulated; positive and negative indices are supported.
class ObjMeshLoader {
public:
    [[nodiscard]] static bool loadFromFile(
        const std::filesystem::path& path, Mesh& output, std::string* error = nullptr);

    [[nodiscard]] static bool loadFromStream(
        std::istream& input, Mesh& output, std::string* error = nullptr);
};

} // namespace originalengine
