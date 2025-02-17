#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <sstream>
#include <iomanip>

/**
 * @brief Represents a 3D vertex.
 */
struct Vertex {
    float x, y, z;
    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(6) << x << " " << y << " " << z;
        return oss.str();
    }
};

/**
 * @brief Represents a triangle defined by three vertex indices.
 */
struct TriangleIndices {
    int v1, v2, v3;
};

/**
 * @brief Represents a triangle after dereferencing the first vertex.
 */
struct Triangle_Pass1 {
    Vertex v1;
    int v2, v3;
};

/**
 * @brief Represents a triangle after dereferencing the second vertex.
 */
struct Triangle_Pass2 {
    Vertex v1, v2;
    int v3;
};

/**
 * @brief Represents a triangle with all vertices dereferenced.
 */
struct TriangleCoordinates {
    Vertex v1, v2, v3;
};

/**
 * @brief Represents a header for a PLY file.
 */
struct PLYHeader {
    std::string format; // "ascii", "binary_little_endian" or "binary_big_endian"
    int vertexCount;
    int faceCount;
    std::streampos headerEndPos;
};

/**
 * @brief Represents a header for an OBJSoup file.
 */
struct ORCOEURHeader {
    std::string format; // "ascii" or "binary_little_endian"
    int faceCount;
};

#endif // STRUCTURES_H
