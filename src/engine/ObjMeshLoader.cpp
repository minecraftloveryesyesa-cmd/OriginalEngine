#include "engine/ObjMeshLoader.hpp"

#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace originalengine {
namespace {

void setError(std::string* error, const std::string& message) {
    if (error != nullptr) {
        *error = message;
    }
}

bool parseIndexToken(const std::string& token, long long& value) {
    const std::size_t slash = token.find('/');
    const std::string indexText = token.substr(0, slash);
    if (indexText.empty()) {
        return false;
    }
    const char* first = indexText.data();
    const char* last = first + indexText.size();
    const auto result = std::from_chars(first, last, value);
    return result.ec == std::errc{} && result.ptr == last && value != 0;
}

bool resolveIndex(long long objIndex, std::size_t vertexCount, std::size_t& index) {
    if (objIndex > 0) {
        const auto positive = static_cast<unsigned long long>(objIndex);
        if (positive > vertexCount) {
            return false;
        }
        index = static_cast<std::size_t>(positive - 1ULL);
        return true;
    }

    const auto magnitude = static_cast<unsigned long long>(-(objIndex + 1LL)) + 1ULL;
    if (magnitude > vertexCount) {
        return false;
    }
    index = vertexCount - static_cast<std::size_t>(magnitude);
    return true;
}

} // namespace

bool ObjMeshLoader::loadFromFile(
    const std::filesystem::path& path, Mesh& output, std::string* error) {
    std::ifstream file(path);
    if (!file) {
        setError(error, "Could not open OBJ file: " + path.string());
        return false;
    }
    return loadFromStream(file, output, error);
}

bool ObjMeshLoader::loadFromStream(
    std::istream& input, Mesh& output, std::string* error) {
    Mesh parsed;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line)) {
        ++lineNumber;
        std::istringstream row(line);
        std::string kind;
        if (!(row >> kind) || kind.starts_with('#')) {
            continue;
        }

        if (kind == "v") {
            MeshVertex vertex;
            if (!(row >> vertex.x >> vertex.y >> vertex.z)) {
                setError(error, "Invalid vertex at line " + std::to_string(lineNumber));
                return false;
            }
            parsed.vertices.push_back(vertex);
        } else if (kind == "f") {
            std::vector<std::size_t> face;
            std::string token;
            while (row >> token) {
                if (token.starts_with('#')) {
                    break;
                }
                long long objIndex = 0;
                std::size_t index = 0;
                if (!parseIndexToken(token, objIndex) ||
                    !resolveIndex(objIndex, parsed.vertices.size(), index)) {
                    setError(error, "Invalid face index at line " + std::to_string(lineNumber));
                    return false;
                }
                face.push_back(index);
            }

            if (face.size() < 3) {
                setError(error, "Face has fewer than three vertices at line " +
                                std::to_string(lineNumber));
                return false;
            }

            for (std::size_t i = 1; i + 1 < face.size(); ++i) {
                parsed.triangles.push_back({{face[0], face[i], face[i + 1]}});
            }
        }
        // Other standard OBJ records (vt, vn, o, g, s, usemtl, mtllib) are ignored.
    }

    if (input.bad()) {
        setError(error, "I/O error while reading OBJ data");
        return false;
    }
    if (parsed.vertices.empty() || parsed.triangles.empty()) {
        setError(error, "OBJ contains no usable triangle geometry");
        return false;
    }

    output = std::move(parsed);
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

} // namespace originalengine
