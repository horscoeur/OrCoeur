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

    // Operator overloads for vector addition
    [[nodiscard]]
    Vertex operator+(const Vertex &v) const {
        return {x + v.x, y + v.y, z + v.z};
    }
    // Operator overloads for vector subtraction
    [[nodiscard]]
    Vertex operator-(const Vertex& v) const{
        return {x-v.x, y-v.y, z-v.z};
    }
};

struct Vertex4 {
    float x, y, z, w;
    [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(10) << x << " " << y << " " << z << " " << w;
        return oss.str();
    }
    // Constructor for the Vertex4 struct with Vertex
    Vertex4(const Vertex &v) : x(v.x), y(v.y), z(v.z), w(0) {}
};

/**
 * @brief Represents a grid cell.
 */
struct Grid {

    explicit Grid(int resolution) : resolution(resolution), min(0), max(0) {}

    Vertex min, max;
    int resolution;

    // getCellX, getCellY, getCellZ and getIndex functions
    // are used to get the index of the cell in the grid

    [[nodiscard]]
    int getCellX(const Vertex pos) const {
        return resolution * (pos.x - min.x) / (max.x - min.x);
    }

    [[nodiscard]]
    int getCellY(const Vertex pos) const {
        return resolution * (pos.y - min.y) / (max.y - min.y);
    }

    [[nodiscard]]
    int getCellZ(const Vertex pos) const {
        return resolution * (pos.z - min.z) / (max.z - min.z);
    }

    [[nodiscard]]
    int getIndex(const int i, const int j, const int k) const {
        return i * resolution * resolution + j * resolution + k;
    }

    [[nodiscard]]
    int getIndex(const Vertex pos) const {
        return getIndex(getCellX(pos), getCellY(pos), getCellZ(pos));
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
