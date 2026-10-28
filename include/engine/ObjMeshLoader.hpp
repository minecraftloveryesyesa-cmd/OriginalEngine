#pragma once

#include "engine/Mesh.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>

namespace originalengine {

// Loads the geometry subset of Wavefront OBJ: v and f records.
// Texture coordinates and normals are ignored; polygon faces are triangulated.
class ObjMeshLoader {
public:
    [[nodiscard]] static bool loadFromFile(
        const std::filesystem::path& path, Mesh& output, std::string* error = nullptr);

    [[nodiscard]] static bool loadFromStream(
        std::istream& input, Mesh& output, std::string* error = nullptr);
};

} // namespace originalengine
