#ifndef CONVERSION_H
#define CONVERSION_H

#include <string>

// Maximum number of triangles loaded in memory to sort a run.
#define CHUNK_SIZE 1000000

// ---------------- Basic Structures ----------------

/**
 * @brief Represents a 3D vertex.
 */
struct Vertex {
    double x, y, z;
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

// ---------------- Conversion Functions ----------------

/**
 * @brief Converts an OBJ file to an OBJSoup format.
 *
 * The function reads the OBJ file, writes the vertices to a binary file and
 * the triangle indices to another binary file. It then performs external sorting
 * on the triangles and dereferences the indices to produce the final output.
 *
 * @param objFilename Input OBJ file path.
 * @param outputFilename Output OBJSoup file path.
 * @param tempVertexFile Path for temporary binary vertex file.
 * @param tempTriangleIndicesFile Path for temporary binary triangle file.
 */
void convertOBJtoOBJSoup(const std::string &objFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempTriangleIndicesFile);

/**
 * @brief Converts a PLY file to an OBJSoup format.
 *
 * The function reads the PLY file, writes the vertices to a binary file and
 * the triangle indices to another binary file. It then performs external sorting
 * on the triangles and dereferences the indices to produce the final output.
 *
 * @param plyFilename Input PLY file path.
 * @param outputFilename Output OBJSoup file path.
 * @param tempVertexFile Path for temporary binary vertex file.
 * @param tempTriangleIndicesFile Path for temporary binary triangle file.
 */
void convertPLYtoOBJSoup(const std::string &plyFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempTriangleIndicesFile);

/**
 * @brief Converts an OBJSoup file back to an OBJ file.
 *
 * This function converts an OBJSoup file (where each face is defined by three vertex
 * coordinates) back into an OBJ file that contains a unique vertex list and face definitions
 * referencing those vertices. The processing is performed in an out-of-core fashion.
 *
 * @param inputFilename Input OBJSoup file path.
 * @param outputFilename Output OBJ file path.
 * @param tempVertexFile Path for a temporary file for vertices.
 * @param tempFaceFile Path for a temporary file for face definitions.
 * @param bufferSize The size of the buffer used for reading (in bytes).
 */
void convertOBJSoupToOBJ(const std::string &inputFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempFaceFile, size_t bufferSize = 1024 * 1024 * 2);

#endif // CONVERSION_H