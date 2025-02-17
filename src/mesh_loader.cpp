#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <array>
#include <string>
#include <cstdint>

#include "utility.h"

bool extractVerticesAndFacesFromOBJ(const std::string &filename, std::vector<std::array<float, 3>> &vertices,
                                    std::vector<std::array<int, 3>> &faces) {
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << ".\n";
        return false;
    }

    vertices.reserve(100000);
    faces.reserve(100000);

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        // If the line starts with 'v', it is a vertex
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            char dummy;
            std::istringstream iss(line);
            iss >> dummy >> x >> y >> z;
            vertices.emplace_back(std::array{x, y, z});
        }

        // If the line starts with 'f', it is a face
        else if (line[0] == 'f' && line[1] == ' ') {
            std::istringstream iss(line.substr(2));
            std::vector<int> faceIndices;
            std::string token;
            while (iss >> token) {
                size_t pos = token.find('/');
                int index = std::stoi(token.substr(0, pos)) - 1;
                faceIndices.push_back(index);
            }

            // Triangulate the face if it has more than 3 vertices else add it as is
            if (faceIndices.size() == 3) {
                faces.emplace_back(std::array{faceIndices[0], faceIndices[1], faceIndices[2]});
            } else {
                // Fan triangulation
                for (size_t i = 1; i < faceIndices.size() - 1; ++i) {
                    faces.emplace_back(std::array{faceIndices[0], faceIndices[i], faceIndices[i + 1]});
                }
            }
        }
    }

    // Resize the vectors to fit the actual number of elements
    vertices.resize(vertices.size());
    faces.resize(faces.size());

    return true;
}

bool extractVerticesAndFacesFromPLY(const std::string &filename, std::vector<std::array<float, 3>> &vertices,
                                    std::vector<std::array<int, 3>> &faces) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open " << filename << "\n";
        return false;
    }

    std::string line;
    bool isBinary = false, isBigEndian = false;
    int vertexCount = 0, faceCount = 0;

    // Reading the header (always in ASCII)
    while (std::getline(file, line)) {
        if (line.starts_with("format")) {
            if (line.find("ascii") != std::string::npos) {
                isBinary = false;
            } else if (line.find("binary_little_endian") != std::string::npos) {
                isBinary = true;
                isBigEndian = false;
            } else if (line.find("binary_big_endian") != std::string::npos) {
                isBinary = true;
                isBigEndian = true;
            }
        } else if (line.starts_with("element vertex")) {
            vertexCount = std::stoi(line.substr(15));  // "element vertex X"
        } else if (line.starts_with("element face")) {
            faceCount = std::stoi(line.substr(13));  // "element face X"
        } else if (line == "end_header") {
            break;
        }
    }

    // Reserve memory for the vertices and faces
    vertices.reserve(vertexCount);
    faces.reserve(faceCount);

    // Reading vertices
    if (!isBinary) {
        for (int i = 0; i < vertexCount; i++) {
            std::getline(file, line);
            const char *vertex = line.c_str();
            char *end;
            float x = std::strtof(vertex, &end);
            float y = std::strtof(end, &end);
            float z = std::strtof(end, nullptr);
            vertices.emplace_back(std::array{x, y, z});
        }
    } else {
        // Binary Mode
        std::vector<char> buffer(vertexCount * sizeof(float) * 3);
        file.read(buffer.data(), buffer.size());
        auto *data = reinterpret_cast<float *>(buffer.data());
        for (int i = 0; i < vertexCount; i++) {
            float x = data[i * 3], y = data[i * 3 + 1], z = data[i * 3 + 2];
            if (isBigEndian) {
                x = swapFloat(x);
                y = swapFloat(y);
                z = swapFloat(z);
            }
            vertices.emplace_back(std::array{x, y, z});
        }
    }

    // Reading faces
    if (!isBinary) {
        for (int i = 0; i < faceCount; i++) {
            std::getline(file, line);
            std::istringstream iss(line);
            int vertexPerFace;
            iss >> vertexPerFace;
            std::vector<int> indices(vertexPerFace);
            for (int j = 0; j < vertexPerFace; j++) {
                iss >> indices[j];
            }

            // Triangulate the face if it has more than 3 vertices else add it as is
            if (vertexPerFace == 3) {
                faces.emplace_back(std::array{indices[0], indices[1], indices[2]});
            } else {
                for (int j = 1; j < vertexPerFace - 1; j++) {
                    faces.emplace_back(std::array{indices[0], indices[j], indices[j + 1]});
                }
            }
        }
    } else {
        // Binary Mode
        for (int i = 0; i < faceCount; i++) {
            uint8_t vertexPerFace;
            file.read(reinterpret_cast<char *>(&vertexPerFace), sizeof(uint8_t));
            if (vertexPerFace < 3) {
                file.seekg(vertexPerFace * sizeof(int), std::ios::cur);
                continue;
            }
            std::vector<int> indices(vertexPerFace);
            file.read(reinterpret_cast<char *>(indices.data()), vertexPerFace * sizeof(int));

            // Swap the byte order if the file is big endian
            if (isBigEndian) {
                for (int &index : indices) {
                    index = static_cast<int>(swapUInt32(static_cast<uint32_t>(index)));
                }
            }

            if (vertexPerFace == 3) {
                faces.emplace_back(std::array{indices[0], indices[1], indices[2]});
            } else {
                for (int j = 1; j < vertexPerFace - 1; j++) {
                    faces.emplace_back(std::array{indices[0], indices[j], indices[j + 1]});
                }
            }
        }
    }

    return true;
}