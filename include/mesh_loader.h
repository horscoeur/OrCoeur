#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include <vector>
#include <array>
#include <string>

/**
 * @brief Extracts vertices and faces from an OBJ file.
 *
 * This function reads an OBJ file and extracts the vertices and faces into the provided vectors.
 *
 * @param filename The path to the OBJ file.
 * @param vertices A vector to store the extracted vertices.
 * @param faces A vector to store the extracted faces.
 * @return true if the extraction was successful, false otherwise.
 */
bool extractVerticesAndFacesFromOBJ(const std::string &filename, std::vector<std::array<float, 3>> &vertices,
                                    std::vector<std::array<int, 3>> &faces);

/**
 * @brief Extracts vertices and faces from a PLY file.
 *
 * This function reads a PLY file and extracts the vertices and faces into the provided vectors.
 *
 * @param filename The path to the PLY file.
 * @param vertices A vector to store the extracted vertices.
 * @param faces A vector to store the extracted faces.
 * @return true if the extraction was successful, false otherwise.
 */
bool extractVerticesAndFacesFromPLY(const std::string &filename, std::vector<std::array<float, 3>> &vertices,
                                    std::vector<std::array<int, 3>> &faces);

#endif // MESH_LOADER_H