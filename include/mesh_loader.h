#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include <vector>
#include <array>
#include <string>

/**
 * @brief Loads an OBJ file and extracts the mesh data.
 * @param filename Path to the OBJ file.
 * @param vertices Output list of vertex positions.
 * @param faces Output list of triangular faces (indices).
 * @return true if the file was successfully loaded, false otherwise.
 */
bool loadOBJFile(const std::string& filename, std::vector<std::array<double, 3>>& vertices, std::vector<std::array<int, 3>>& faces);

#endif // MESH_LOADER_H
