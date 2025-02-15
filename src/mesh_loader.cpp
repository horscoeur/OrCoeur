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
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << ".\n";
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.size() < 2) continue;

        // Vertex extraction
        if (line.substr(0, 2) == "v ") {
            std::istringstream iss(line.substr(2));
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                std::cerr << "Error: Could not parse vertex line: " << line << ".\n";
                continue;
            }
            vertices.push_back({x, y, z});
        }

        // Face extraction
        else if (line.substr(0, 2) == "f ") {
            std::istringstream iss(line.substr(2));
            std::vector<int> faceIndices;
            std::string token;
            while (iss >> token) {
                // "v" or "v/vt/vn" format token: we take the first value
                std::istringstream tokenStream(token);
                std::string indexStr;
                if (std::getline(tokenStream, indexStr, '/')) {
                    int index = std::stoi(indexStr);
                    // Conversion of 1-based index to 0-based index
                    faceIndices.push_back(index - 1);
                }
            }
            // If the face is already a triangle, we add it as is
            if (faceIndices.size() == 3) {
                faces.push_back({faceIndices[0], faceIndices[1], faceIndices[2]});
            }
            // Else, we triangulate the face (fan triangulation method)
            else if (faceIndices.size() > 3) {
                for (size_t i = 1; i < faceIndices.size() - 1; i++) {
                    faces.push_back({faceIndices[0], faceIndices[i], faceIndices[i + 1]});
                }
            }
        }
    }
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
    bool isBinary = false;
    bool isBigEndian = false;
    int vertexCount = 0;
    int faceCount = 0;

    // Reading the header (always in ASCII)
    while (std::getline(file, line)) {
        if (line.substr(0, 6) == "format") {
            if (line.find("ascii") != std::string::npos) {
                isBinary = false;
            } else if (line.find("binary_little_endian") != std::string::npos) {
                isBinary = true;
                isBigEndian = false;
            } else if (line.find("binary_big_endian") != std::string::npos) {
                isBinary = true;
                isBigEndian = true;
            }
        } else if (line.substr(0, 14) == "element vertex") {
            std::istringstream iss(line);
            std::string elem, vertexStr;
            iss >> elem >> vertexStr >> vertexCount;
        } else if (line.substr(0, 12) == "element face") {
            std::istringstream iss(line);
            std::string elem, faceStr;
            iss >> elem >> faceStr >> faceCount;
        } else if (line == "end_header") {
            break;
        }
    }

    // Reading vertices
    if (!isBinary) {
        // ASCII mode
        for (int i = 0; i < vertexCount; i++) {
            std::getline(file, line);
            std::istringstream iss(line);
            float x, y, z;
            if (!(iss >> x >> y >> z)) {
                std::cerr << "Error reading a vertex.\n";
                continue;
            }
            vertices.push_back({x, y, z});
        }
    } else {
        // Binary mode (assuming coordinates are stored as floats)
        for (int i = 0; i < vertexCount; i++) {
            float coords[3];
            file.read(reinterpret_cast<char *>(coords), sizeof(float) * 3);
            if (isBigEndian) {
                for (int j = 0; j < 3; j++) {
                    uint32_t temp = *reinterpret_cast<uint32_t *>(&coords[j]);
                    temp = swapUInt32(temp);
                    coords[j] = *reinterpret_cast<float *>(&temp);
                }
            }
            vertices.push_back({coords[0], coords[1], coords[2]});
        }
    }

    // Reading faces
    if (!isBinary) {
        // ASCII mode
        for (int i = 0; i < faceCount; i++) {
            std::getline(file, line);
            std::istringstream iss(line);
            int vertexPerFace;
            if (!(iss >> vertexPerFace)) {
                std::cerr << "Error reading a face.\n";
                continue;
            }
            std::vector<int> indices(vertexPerFace);
            for (int j = 0; j < vertexPerFace; j++) {
                iss >> indices[j];
            }
            if (vertexPerFace == 3) {
                faces.push_back({indices[0], indices[1], indices[2]});
            } else if (vertexPerFace > 3) {
                // Triangulation (fan method)
                for (int j = 1; j < vertexPerFace - 1; j++) {
                    faces.push_back({indices[0], indices[j], indices[j + 1]});
                }
            }
        }
    } else {
        // Binary mode
        for (int i = 0; i < faceCount; i++) {
            uint8_t vertexPerFace;
            file.read(reinterpret_cast<char *>(&vertexPerFace), sizeof(uint8_t));
            if (vertexPerFace < 3) {
                // If the face has less than 3 vertices, ignore it
                file.seekg(vertexPerFace * sizeof(int), std::ios::cur);
                continue;
            }
            std::vector<int> indices(vertexPerFace);
            for (int j = 0; j < vertexPerFace; j++) {
                int index;
                file.read(reinterpret_cast<char *>(&index), sizeof(int));
                if (isBigEndian) {
                    index = static_cast<int>(swapUInt32(static_cast<uint32_t>(index)));
                }
                indices[j] = index;
            }
            if (vertexPerFace == 3) {
                faces.push_back({indices[0], indices[1], indices[2]});
            } else if (vertexPerFace > 3) {
                // Triangulation
                for (int j = 1; j < vertexPerFace - 1; j++) {
                    faces.push_back({indices[0], indices[j], indices[j + 1]});
                }
            }
        }
    }

    return true;
}
