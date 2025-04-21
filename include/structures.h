#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include "polyscope/polyscope.h"
#include "polyscope/point_cloud.h"


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

    void displayGrid() {
        std::vector<Vertex> gridCenters;

        // Loop through the grid and calculate the center of each cell
        for (int i = 0; i < resolution; i++) {
            for (int j = 0; j < resolution; j++) {
                for (int k = 0; k < resolution; k++) {
                    // Calculate the center of the cell
                    float x = min.x + (i + 0.5f) * (max.x - min.x) / resolution;
                    float y = min.y + (j + 0.5f) * (max.y - min.y) / resolution;
                    float z = min.z + (k + 0.5f) * (max.z - min.z) / resolution;

                    gridCenters.emplace_back(Vertex{x, y, z});
                }
            }
        }
        // Register the grid centers with Polyscope
        std::vector<std::array<double, 3>> points;
        for (const auto& v : gridCenters) {
            points.push_back({v.x, v.y, v.z});
        }
        polyscope::registerPointCloud("Grid Centers", points);

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

/**
 * @brief Represents a plane equation in homogeneous coordinates.
 *
 * The plane is defined by the equation:
 *     a*x + b*y + c*z + d = 0,
 * where (a, b, c) is a normalized normal vector and d is the offset.
 */
struct PlaneEquation {
    float a, b, c, d;
};

/**
 * @brief Represents a quadric as a 4x4 matrix stored in a flat array.
 */
struct Quadric {
    std::array<float, 16> data{};

    constexpr float& operator()(const int i, const int j) { return data[i * 4 + j]; }
    constexpr const float& operator()(const int i, const int j) const { return data[i * 4 + j]; }
};


/**
 * @brief Represents the grid index and the plane equation 
 */
struct GridPlaneEntry {
    int gridIndex;
    PlaneEquation planeEquation;
};

/**
 * @brief Represents the triangle and its barycenter
 */
struct TriangleWithBarycenter {
    TriangleCoordinates triangle;
    Vertex barycenter;  
};

#endif // STRUCTURES_H
