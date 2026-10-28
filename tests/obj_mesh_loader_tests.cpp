#include "engine/ObjMeshLoader.hpp"

#include <array>

#include <iostream>
#include <sstream>
#include <string>

int main() {
    originalengine::Mesh mesh;
    std::string error;
    std::istringstream obj(
        "# tiny quad\n"
        "o TestQuad\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/1/1 3/1/1 4/1/1\n");

    if (!originalengine::ObjMeshLoader::loadFromStream(obj, mesh, &error)) {
        std::cerr << "OBJ load failed: " << error << '\n';
        return 1;
    }
    if (mesh.vertices.size() != 4 || mesh.triangles.size() != 2 ||
        mesh.triangles[0].indices != std::array<std::size_t, 3>{0, 1, 2} ||
        mesh.triangles[1].indices != std::array<std::size_t, 3>{0, 2, 3}) {
        std::cerr << "Quad was not parsed and triangulated correctly.\n";
        return 1;
    }

    std::istringstream negativeIndices(
        "v 0 0 0\nv 1 0 0\nv 0 1 0\nf -3 -2 -1\n");
    originalengine::Mesh negativeMesh;
    if (!originalengine::ObjMeshLoader::loadFromStream(negativeIndices, negativeMesh, &error) ||
        negativeMesh.triangles.size() != 1 ||
        negativeMesh.triangles[0].indices != std::array<std::size_t, 3>{0, 1, 2}) {
        std::cerr << "Negative OBJ indices were not handled correctly: " << error << '\n';
        return 1;
    }

    std::istringstream invalid("v 0 0 0\nf 1 2 3\n");
    originalengine::Mesh preserved = mesh;
    if (originalengine::ObjMeshLoader::loadFromStream(invalid, mesh, &error) ||
        mesh.vertices.size() != preserved.vertices.size() ||
        mesh.triangles.size() != preserved.triangles.size()) {
        std::cerr << "Invalid input should fail without replacing the output mesh.\n";
        return 1;
    }

    std::cout << "OBJ mesh loader tests passed.\n";
    return 0;
}
