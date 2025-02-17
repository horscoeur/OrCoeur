#include "dereference_passes.h"
#include "structures.h"
#include "merge_sort.h"

#include <fstream>
#include <iostream>
#include <vector>

#define BUFFER_SIZE 1024


// Specialized external sort functions for each type:

// To sort triangles by the first index (v1)
void externalMergeSortTrianglesIndices(const std::string &inputFile, const std::string &outputFile) {
    externalMergeSort<TriangleIndices>(inputFile, outputFile, [](const TriangleIndices &a, const TriangleIndices &b) {
        return a.v1 < b.v1;
    });
}

// To sort records from pass 1 by the second index (v2)
void externalMergeSortTrianglesPass1(const std::string &inputFile, const std::string &outputFile) {
    externalMergeSort<Triangle_Pass1>(inputFile, outputFile, [](const Triangle_Pass1 &a, const Triangle_Pass1 &b) {
        return a.v2 < b.v2;
    });
}

// To sort records from pass 2 by the third index (v3)
void externalMergeSortTrianglesPass2(const std::string &inputFile, const std::string &outputFile) {
    externalMergeSort<Triangle_Pass2>(inputFile, outputFile, [](const Triangle_Pass2 &a, const Triangle_Pass2 &b) {
        return a.v3 < b.v3;
    });
}

// ----- Dereferencing passes using synchronized scanning -----
// Each pass reads the sorted file on the target index and the vertex file sequentially.

// Pass 1: Replace v1 (an index) with the corresponding Vertex
void dereferencePass1(const std::string &triangleIndexFile, const std::string &vertexFile, const std::string &outputFile) {
    // First, sort the triangle indices by v1
    externalMergeSortTrianglesIndices(triangleIndexFile, "sorted_by_v1.bin");

    // Open files in binary mode
    std::ifstream sortedTriangles("sorted_by_v1.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);
    if (!sortedTriangles.is_open() || !vertices.is_open() || !out.is_open()) {
        std::cerr << "Error: Unable to open one of the files in dereferencePass1." << std::endl;
        std::remove("sorted_by_v1.bin");
        return;
    }

    // Buffers for reading triangle indices and writing output records
    std::vector<TriangleIndices> faceBuffer(BUFFER_SIZE);
    std::vector<Triangle_Pass1> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);

    // Buffer for reading vertices
    std::vector<Vertex> vertexBuffer(BUFFER_SIZE);
    size_t vertexBufferIndex = 0;  // Current position within the vertex buffer
    size_t vertexBufferCount = 0;  // Number of vertices currently in the buffer
    int currentIndex = 1;          // Global vertex index (OBJ indices start at 1)
    Vertex currentVertex;

    // Lambda to refill the vertex buffer from the vertex file
    auto refillVertexBuffer = [&]() -> bool {
        vertices.read(reinterpret_cast<char*>(vertexBuffer.data()), BUFFER_SIZE * sizeof(Vertex));
        vertexBufferCount = vertices.gcount() / sizeof(Vertex);
        vertexBufferIndex = 0;
        return vertexBufferCount > 0;
    };

    // Initial fill of the vertex buffer
    if (!refillVertexBuffer()) {
        std::cerr << "Error: Unable to read the first vertex in dereferencePass1." << std::endl;
        std::remove("sorted_by_v1.bin");
        return;
    }
    currentVertex = vertexBuffer[0];

    // Process the sorted triangle indices in blocks
    while (true) {
        sortedTriangles.read(reinterpret_cast<char*>(faceBuffer.data()), BUFFER_SIZE * sizeof(TriangleIndices));
        size_t facesRead = sortedTriangles.gcount() / sizeof(TriangleIndices);
        if (facesRead == 0)
            break;

        for (size_t i = 0; i < facesRead; i++) {
            const auto &[v1, v2, v3] = faceBuffer[i];

            // Advance through the vertex file until we reach the vertex at index face.v1
            while (currentIndex < v1) {
                vertexBufferIndex++;
                if (vertexBufferIndex >= vertexBufferCount) {
                    if (!refillVertexBuffer()) {
                        std::cerr << "Error: Vertex index " << v1
                                  << " not found in vertex file (dereferencePass1)." << std::endl;
                        std::remove("sorted_by_v1.bin");
                        return;
                    }
                }
                currentVertex = vertexBuffer[vertexBufferIndex];
                currentIndex++;
            }

            // Create a Triangle_Pass1 record with v1 replaced by the actual Vertex
            Triangle_Pass1 rec{};
            rec.v1 = currentVertex;
            rec.v2 = v2;
            rec.v3 = v3;
            outputBuffer.push_back(rec);

            // Write the output buffer to disk when full
            if (outputBuffer.size() >= BUFFER_SIZE) {
                out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(Triangle_Pass1));
                outputBuffer.clear();
            }
        }
    }

    // Write any remaining records in the output buffer
    if (!outputBuffer.empty()) {
        out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(Triangle_Pass1));
    }

    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v1.bin");
}

// Pass 2: Replace v2 (an index) with the corresponding Vertex
void dereferencePass2(const std::string &inputFile, const std::string &vertexFile, const std::string &outputFile) {
    // Sort Triangle_Pass1 records by v2
    externalMergeSortTrianglesPass1(inputFile, "sorted_by_v2.bin");

    std::ifstream sortedTriangles("sorted_by_v2.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);
    if (!sortedTriangles.is_open() || !vertices.is_open() || !out.is_open()) {
        std::cerr << "Error: Unable to open one of the files in dereferencePass2." << std::endl;
        std::remove("sorted_by_v2.bin");
        return;
    }

    // Buffers for reading input records and writing output records
    std::vector<Triangle_Pass1> faceBuffer(BUFFER_SIZE);
    std::vector<Triangle_Pass2> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);

    // Vertex buffering
    std::vector<Vertex> vertexBuffer(BUFFER_SIZE);
    size_t vertexBufferIndex = 0;
    size_t vertexBufferCount = 0;
    int currentIndex = 1; // Reset vertex index for this pass
    Vertex currentVertex{};

    auto refillVertexBuffer = [&]() -> bool {
        vertices.read(reinterpret_cast<char*>(vertexBuffer.data()), BUFFER_SIZE * sizeof(Vertex));
        vertexBufferCount = vertices.gcount() / sizeof(Vertex);
        vertexBufferIndex = 0;
        return vertexBufferCount > 0;
    };

    if (!refillVertexBuffer()) {
        std::cerr << "Error: Unable to read the first vertex in dereferencePass2." << std::endl;
        std::remove("sorted_by_v2.bin");
        return;
    }
    currentVertex = vertexBuffer[0];

    // Process Triangle_Pass1 records in blocks
    while (true) {
        sortedTriangles.read(reinterpret_cast<char*>(faceBuffer.data()), BUFFER_SIZE * sizeof(Triangle_Pass1));
        size_t recordsRead = sortedTriangles.gcount() / sizeof(Triangle_Pass1);
        if (recordsRead == 0)
            break;

        for (size_t i = 0; i < recordsRead; i++) {
            const auto &[v1, v2, v3] = faceBuffer[i];

            // Advance through the vertex file until reaching vertex at index recIn.v2
            while (currentIndex < v2) {
                vertexBufferIndex++;
                if (vertexBufferIndex >= vertexBufferCount) {
                    if (!refillVertexBuffer()) {
                        std::cerr << "Error: Vertex index " << v2
                                  << " not found in vertex file (dereferencePass2)." << std::endl;
                        std::remove("sorted_by_v2.bin");
                        return;
                    }
                }
                currentVertex = vertexBuffer[vertexBufferIndex];
                currentIndex++;
            }

            // Create a Triangle_Pass2 record with v2 replaced by the actual Vertex
            Triangle_Pass2 recOut{};
            recOut.v1 = v1;
            recOut.v2 = currentVertex;
            recOut.v3 = v3;
            outputBuffer.push_back(recOut);

            if (outputBuffer.size() >= BUFFER_SIZE) {
                out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(Triangle_Pass2));
                outputBuffer.clear();
            }
        }
    }

    if (!outputBuffer.empty()) {
        out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(Triangle_Pass2));
    }

    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v2.bin");
}

// Pass 3: Replace v3 (an index) with the corresponding Vertex
void dereferencePass3(const std::string &inputFile, const std::string &vertexFile, const std::string &outputFile) {
    // Sort Triangle_Pass2 records by v3
    externalMergeSortTrianglesPass2(inputFile, "sorted_by_v3.bin");

    std::ifstream sortedTriangles("sorted_by_v3.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);
    if (!sortedTriangles.is_open() || !vertices.is_open() || !out.is_open()) {
        std::cerr << "Error: Unable to open one of the files in dereferencePass3." << std::endl;
        std::remove("sorted_by_v3.bin");
        return;
    }

    // Buffers for reading input records and writing final output records
    std::vector<Triangle_Pass2> faceBuffer(BUFFER_SIZE);
    std::vector<TriangleCoordinates> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);

    // Buffer for reading vertices
    std::vector<Vertex> vertexBuffer(BUFFER_SIZE);
    size_t vertexBufferIndex = 0;
    size_t vertexBufferCount = 0;
    int currentIndex = 1;
    Vertex currentVertex{};

    auto refillVertexBuffer = [&]() -> bool {
        vertices.read(reinterpret_cast<char*>(vertexBuffer.data()), BUFFER_SIZE * sizeof(Vertex));
        vertexBufferCount = vertices.gcount() / sizeof(Vertex);
        vertexBufferIndex = 0;
        return vertexBufferCount > 0;
    };

    if (!refillVertexBuffer()) {
        std::cerr << "Error: Unable to read the first vertex in dereferencePass3." << std::endl;
        std::remove("sorted_by_v3.bin");
        return;
    }
    currentVertex = vertexBuffer[0];

    // Process Triangle_Pass2 records in blocks
    while (true) {
        sortedTriangles.read(reinterpret_cast<char*>(faceBuffer.data()), BUFFER_SIZE * sizeof(Triangle_Pass2));
        size_t recordsRead = sortedTriangles.gcount() / sizeof(Triangle_Pass2);
        if (recordsRead == 0)
            break;

        for (size_t i = 0; i < recordsRead; i++) {
            const auto &[v1, v2, v3] = faceBuffer[i];

            // Advance through the vertex file until reaching vertex at index recIn.v3
            while (currentIndex < v3) {
                vertexBufferIndex++;
                if (vertexBufferIndex >= vertexBufferCount) {
                    if (!refillVertexBuffer()) {
                        std::cerr << "Error: Vertex index " << v3
                                  << " not found in vertex file (dereferencePass3)." << std::endl;
                        std::remove("sorted_by_v3.bin");
                        return;
                    }
                }
                currentVertex = vertexBuffer[vertexBufferIndex];
                currentIndex++;
            }

            // Create a TriangleCoordinates record with v3 replaced by the actual Vertex
            TriangleCoordinates recOut{};
            recOut.v1 = v1;
            recOut.v2 = v2;
            recOut.v3 = currentVertex;
            outputBuffer.push_back(recOut);

            if (outputBuffer.size() >= BUFFER_SIZE) {
                out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(TriangleCoordinates));
                outputBuffer.clear();
            }
        }
    }

    if (!outputBuffer.empty()) {
        out.write(reinterpret_cast<char*>(outputBuffer.data()), outputBuffer.size() * sizeof(TriangleCoordinates));
    }

    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v3.bin");
}