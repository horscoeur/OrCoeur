#define TINYOBJLOADER_IMPLEMENTATION

#include "mesh_loader.h"
#include "tiny_obj_loader.h"
#include <iostream>

bool loadOBJFile(const std::string& filename, std::vector<std::array<double, 3>>& vertices, std::vector<std::array<int, 3>>& faces) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Load OBJ file
    bool success = LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str());
    if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;
    if (!err.empty()) std::cerr << "ERR: " << err << std::endl;
    if (!success) {
        std::cerr << "Failed to load OBJ file: " << filename << std::endl;
        return false;
    }

    // Convert data format
    vertices.clear();
    faces.clear();
    for (size_t i = 0; i < attrib.vertices.size() / 3; ++i) {
        vertices.push_back({attrib.vertices[3 * i], attrib.vertices[3 * i + 1], attrib.vertices[3 * i + 2]});
    }

    for (const auto& shape : shapes) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
            if (shape.mesh.num_face_vertices[f] != 3) {
                std::cerr << "Non-triangular face detected. Skipping..." << std::endl;
                continue;
            }

            std::array<int, 3> face{};
            for (size_t v = 0; v < 3; ++v) {
                face[v] = shape.mesh.indices[index_offset + v].vertex_index;
            }
            faces.push_back(face);
            index_offset += 3;
        }
    }

    return true;
}
