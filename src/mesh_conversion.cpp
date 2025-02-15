#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "mesh_conversion.h"


// ---------------- Operator Overloads ----------------

bool operator<(const TriangleIndices &a, const TriangleIndices &b) {
    if (a.v1 != b.v1) return a.v1 < b.v1;
    if (a.v2 != b.v2) return a.v2 < b.v2;
    return a.v3 < b.v3;
}

// ----- Generic external sort function -----
// Reads the input file in chunks, sorts each chunk in RAM,
// writes sorted "runs", then performs a k-way merge.
template<typename T, typename Comparator>
void externalMergeSort(const std::string &inputFile, const std::string &outputFile, Comparator comp) {
    // Phase 1: Create sorted runs
    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Error: Unable to open " << inputFile << std::endl;
        return;
    }
    std::vector<std::string> runFiles;
    int runCount = 0;

    while (true) {
        std::vector<T> buffer;
        buffer.resize(CHUNK_SIZE);
        int count = 0;
        while (count < CHUNK_SIZE && in.read(reinterpret_cast<char *>(&buffer[count]), sizeof(T))) {
            count++;
        }
        if (count == 0) break;
        buffer.resize(count);
        std::sort(buffer.begin(), buffer.end(), comp);
        std::string runFileName = "run_" + std::to_string(runCount) + ".bin";
        std::ofstream runFile(runFileName, std::ios::binary);
        if (!runFile) {
            std::cerr << "Error: Unable to write " << runFileName << std::endl;
            return;
        }
        runFile.write(reinterpret_cast<char *>(buffer.data()), count * sizeof(T));
        runFile.close();
        runFiles.push_back(runFileName);
        runCount++;
        if (in.eof()) break;
    }
    in.close();

    // Phase 2: k-way merge using a heap
    struct HeapNode {
        T record;
        int runIndex; // index of the run in runFiles
    };
    auto heapComparator = [comp](const HeapNode &a, const HeapNode &b) {
        return comp(b.record, a.record); // for a min-heap
    };
    std::vector<HeapNode> heap;

    // Open all run files for reading
    std::vector<std::ifstream *> runStreams;
    for (size_t i = 0; i < runFiles.size(); i++) {
        auto *stream = new std::ifstream(runFiles[i], std::ios::binary);
        if (!stream->is_open()) {
            std::cerr << "Error: Unable to open " << runFiles[i] << std::endl;
            return;
        }
        runStreams.push_back(stream);
        T rec{};
        if (stream->read(reinterpret_cast<char *>(&rec), sizeof(T))) {
            heap.push_back({rec, static_cast<int>(i)});
        }
    }
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Unable to open " << outputFile << std::endl;
        return;
    }
    while (!heap.empty()) {
        std::pop_heap(heap.begin(), heap.end(), heapComparator);
        HeapNode node = heap.back();
        heap.pop_back();
        out.write(reinterpret_cast<char*>(&node.record), sizeof(T));
        int idx = node.runIndex;
        T rec{};
        if (runStreams[idx]->read(reinterpret_cast<char*>(&rec), sizeof(T))) {
            heap.push_back({rec, idx});
            std::push_heap(heap.begin(), heap.end(), heapComparator);
        }
    }
    out.close();

    // Close and clean up streams and delete temporary files
    for (size_t i = 0; i < runStreams.size(); i++) {
        runStreams[i]->close();
        delete runStreams[i];
        std::remove(runFiles[i].c_str());
    }
}

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

// Pass 1: Replace v1 with the coordinate
void dereferencePass1(const std::string &triangleIndexFile, const std::string &vertexFile, const std::string &outputFile) {
    // External sort on the v1 field
    externalMergeSortTrianglesIndices(triangleIndexFile, "sorted_by_v1.bin");
    std::ifstream sortedTriangles("sorted_by_v1.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);

    TriangleIndices tri{};
    Vertex currentVertex{};
    int currentIndex = 1; // OBJ indices start at 1

    if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first vertex (pass1)." << std::endl;
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&tri), sizeof(TriangleIndices))) {
        // Advance in the vertices until the vertex corresponding to tri.v1 is found
        while (currentIndex < tri.v1) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << tri.v1 << " not found in the vertex file." << std::endl;
                return;
            }
            currentIndex++;
        }
        // Write a Triangle_Pass1 record with the dereferenced v1
        Triangle_Pass1 rec{};
        rec.v1 = currentVertex;
        rec.v2 = tri.v2;
        rec.v3 = tri.v3;
        out.write(reinterpret_cast<char*>(&rec), sizeof(Triangle_Pass1));
    }
    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v1.bin");
}

// Pass 2: Replace v2 with its coordinate
void dereferencePass2(const std::string &inputFile, const std::string &vertexFile, const std::string &outputFile) {
    // inputFile contains Triangle_Pass1 records.
    externalMergeSortTrianglesPass1(inputFile, "sorted_by_v2.bin");
    std::ifstream sortedTriangles("sorted_by_v2.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);

    Triangle_Pass1 recIn{};
    Vertex currentVertex{};
    int currentIndex = 1;

    if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first vertex (pass2)." << std::endl;
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&recIn), sizeof(Triangle_Pass1))) {
        while (currentIndex < recIn.v2) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << recIn.v2 << " not found (pass2)." << std::endl;
                return;
            }
            currentIndex++;
        }
        Triangle_Pass2 recOut{};
        recOut.v1 = recIn.v1;
        recOut.v2 = currentVertex;
        recOut.v3 = recIn.v3;
        out.write(reinterpret_cast<char*>(&recOut), sizeof(Triangle_Pass2));
    }
    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v2.bin");
}

// Pass 3: Replace v3 with its coordinate
void dereferencePass3(const std::string &inputFile, const std::string &vertexFile, const std::string &outputFile) {
    // inputFile contains Triangle_Pass2 records.
    externalMergeSortTrianglesPass2(inputFile, "sorted_by_v3.bin");
    std::ifstream sortedTriangles("sorted_by_v3.bin", std::ios::binary);
    std::ifstream vertices(vertexFile, std::ios::binary);
    std::ofstream out(outputFile, std::ios::binary);

    Triangle_Pass2 recIn{};
    Vertex currentVertex{};
    int currentIndex = 1;

    if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first vertex (pass3)." << std::endl;
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&recIn), sizeof(Triangle_Pass2))) {
        while (currentIndex < recIn.v3) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << recIn.v3 << " not found (pass3)." << std::endl;
                return;
            }
            currentIndex++;
        }
        TriangleCoordinates recOut{};
        recOut.v1 = recIn.v1;
        recOut.v2 = recIn.v2;
        recOut.v3 = currentVertex;
        out.write(reinterpret_cast<char*>(&recOut), sizeof(TriangleCoordinates));
    }
    sortedTriangles.close();
    vertices.close();
    out.close();
    std::remove("sorted_by_v3.bin");
}

// ------------------- Conversion from OBJ to OBJSoup -------------------
// It first writes temporary binary files for vertices and triangle indices,
// then performs the three dereferencing passes to obtain the final text file.
void convertOBJtoOBJSoup(const std::string &objFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempTriangleIndicesFile) {
    // Phase 1: Convert OBJ to two temporary binary files
    std::ifstream objFile(objFilename);
    std::ofstream vertexFile(tempVertexFile, std::ios::binary);
    std::ofstream triangleIndicesFile(tempTriangleIndicesFile, std::ios::binary);

    auto start = std::chrono::high_resolution_clock::now();

    if (!objFile.is_open() || !vertexFile.is_open() || !triangleIndicesFile.is_open()) {
        std::cerr << "Error: Unable to open temporary files." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(objFile, line)) {
        if (line.empty())
            continue;
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        if (type == "v") {
            Vertex v{};
            iss >> v.x >> v.y >> v.z;
            vertexFile.write(reinterpret_cast<const char *>(&v), sizeof(Vertex));
        } else if (type == "f") {
            TriangleIndices t{};
            iss >> t.v1 >> t.v2 >> t.v3;
            triangleIndicesFile.write(reinterpret_cast<const char *>(&t), sizeof(TriangleIndices));
        }
    }
    objFile.close();
    vertexFile.close();
    triangleIndicesFile.close();

    // Phase 2: The 3 dereferencing passes
    // Pass 1: Dereference v1
    dereferencePass1(tempTriangleIndicesFile, tempVertexFile, "triangles_pass1.bin");
    // Pass 2: Dereference v2
    dereferencePass2("triangles_pass1.bin", tempVertexFile, "triangles_pass2.bin");
    // Pass 3: Dereference v3
    dereferencePass3("triangles_pass2.bin", tempVertexFile, "triangles_final.bin");

    // Phase 3: Write the final OBJSoup file (text format)
    std::ifstream finalTriangles("triangles_final.bin", std::ios::binary);
    std::ofstream outFile(outputFilename);
    if (!finalTriangles.is_open() || !outFile.is_open()) {
        std::cerr << "Error: Unable to open final file." << std::endl;
        return;
    }
    TriangleCoordinates tri{};
    while (finalTriangles.read(reinterpret_cast<char *>(&tri), sizeof(TriangleCoordinates))) {
        outFile << "f " << tri.v1.toString() << " " << tri.v2.toString() << " " << tri.v3.toString() << "\n";
    }
    finalTriangles.close();
    outFile.close();

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
              << "Output written to " << outputFilename << std::endl;

    // Delete temporary files
    std::remove(tempTriangleIndicesFile.c_str());
    std::remove("triangles_pass1.bin");
    std::remove("triangles_pass2.bin");
    std::remove("triangles_final.bin");
}


// ---------------- Conversion from OBJSoup to OBJ ----------------

/**
 * @brief Converts an OBJSoup file back to a standard OBJ file.
 *
 * This implementation reads the input file in chunks (using a specified buffer size)
 * and processes the file sequentially. For each face line in the OBJSoup, it writes the
 * vertex definitions to a temporary file and creates face definitions with direct indices.
 *
 * Finally, it merges the temporary vertex and face files into the final OBJ output.
 */
void convertOBJSoupToOBJ(const std::string &inputFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempFaceFile, size_t bufferSize) {
    std::ifstream objFile(inputFilename, std::ios::binary);
    std::ofstream vertexFile(tempVertexFile, std::ios::binary);
    std::ofstream faceFile(tempFaceFile);

    auto start = std::chrono::high_resolution_clock::now();

    if (!objFile.is_open() || !vertexFile.is_open() || !faceFile.is_open()) {
        std::cerr << "Error: Unable to open file(s)." << std::endl;
        return;
    }

    // Use a buffer to read chunks from the file.
    std::vector<char> buffer(bufferSize);
    std::string remaining;
    int nextIndex = 1; // OBJ vertex indices start at 1

    // Process the file in chunks.
    while (objFile.read(buffer.data(), buffer.size()) || objFile.gcount() > 0) {
        size_t bytesRead = objFile.gcount();
        std::string chunk = remaining + std::string(buffer.data(), bytesRead);

        size_t lastNewline = chunk.rfind('\n');
        if (lastNewline == std::string::npos) {
            remaining = chunk;
            continue;
        }

        remaining = chunk.substr(lastNewline + 1);
        std::istringstream chunkStream(chunk.substr(0, lastNewline + 1));
        std::string line;

        while (std::getline(chunkStream, line)) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "f") {
                Vertex v1{}, v2{}, v3{};
                iss >> v1.x >> v1.y >> v1.z >> v2.x >> v2.y >> v2.z >> v3.x >> v3.y >> v3.z;

                // Write vertex definitions to the temporary vertex file.
                vertexFile << "v " << v1.toString() << "\n";
                vertexFile << "v " << v2.toString() << "\n";
                vertexFile << "v " << v3.toString() << "\n";

                // Write the face definition with direct indices.
                faceFile << "f " << nextIndex << " " << (nextIndex + 1) << " " << (nextIndex + 2) << "\n";
                nextIndex += 3;
            }
        }
    }

    objFile.close();
    vertexFile.close();
    faceFile.close();

    // Merge temporary vertex and face files into the final OBJ output.
    std::ofstream finalOutFile(outputFilename);
    std::ifstream vertexFileIn(tempVertexFile);
    std::ifstream faceFileIn(tempFaceFile);

    if (!vertexFileIn.is_open() || !faceFileIn.is_open() || !finalOutFile.is_open()) {
        std::cerr << "Error: Unable to open intermediate file(s)." << std::endl;
        return;
    }

    // Write a header comment and then copy the contents.
    finalOutFile << "# OrCoeur - Converted OBJ file\n";
    finalOutFile << vertexFileIn.rdbuf();
    finalOutFile << faceFileIn.rdbuf();

    vertexFileIn.close();
    faceFileIn.close();
    finalOutFile.close();

    // Remove temporary files.
    std::remove(tempVertexFile.c_str());
    std::remove(tempFaceFile.c_str());

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
            << "Output written to " << outputFilename << std::endl;
}


// ------------------- Conversion from PLY to OBJSoup -------------------
// To minimize duplication, we first extract the vertices and faces into temporary binary files,
// in the same way as for the OBJ format.

bool parsePLYHeader(std::istream &in, PLYHeader &header) {
    std::string line;
    if (!std::getline(in, line) || line != "ply") {
        std::cerr << "Erreur: le fichier doit commencer par 'ply'" << std::endl;
        return false;
    }
    header.vertexCount = 0;
    header.faceCount = 0;
    header.format = "ascii"; // default format
    while (std::getline(in, line)) {
        if (line.substr(0, 6) == "format") {
            std::istringstream iss(line);
            std::string dummy, fmt;
            iss >> dummy >> fmt;
            header.format = fmt;
        } else if (line.substr(0, 14) == "element vertex") {
            std::istringstream iss(line);
            std::string dummy;
            iss >> dummy; // "element"
            iss >> dummy; // "vertex"
            iss >> header.vertexCount;
        } else if (line.substr(0, 12) == "element face") {
            std::istringstream iss(line);
            std::string dummy;
            iss >> dummy; // "element"
            iss >> dummy; // "face"
            iss >> header.faceCount;
        } else if (line == "end_header") {
            header.headerEndPos = in.tellg();
            break;
        }
    }
    return true;
}

void processPLYFile(const std::string &plyFilename, const std::string &tempVertexFile, const std::string &tempTriangleIndicesFile) {
    // Open in binary mode to read the header (which is in ascii)
    std::ifstream plyFile(plyFilename, std::ios::in | std::ios::binary);
    if (!plyFile.is_open()) {
        std::cerr << "Error: Unable to open " << plyFilename << std::endl;
        return;
    }
    PLYHeader header;
    if (!parsePLYHeader(plyFile, header)) {
        std::cerr << "Error: Invalid PLY header." << std::endl;
        return;
    }
    bool isAscii = (header.format == "ascii");

    std::ofstream vertexOut(tempVertexFile, std::ios::binary);
    std::ofstream faceOut(tempTriangleIndicesFile, std::ios::binary);
    if (!vertexOut.is_open() || !faceOut.is_open()) {
        std::cerr << "Error: Unable to open temporary files." << std::endl;
        return;
    }

    if (isAscii) {
        // Reading vertices in ascii format
        std::string line;
        for (int i = 0; i < header.vertexCount; i++) {
            if (!std::getline(plyFile, line)) {
                std::cerr << "Error: Failed to read vertices." << std::endl;
                return;
            }
            std::istringstream iss(line);
            Vertex v{};
            iss >> v.x >> v.y >> v.z;
            vertexOut.write(reinterpret_cast<const char*>(&v), sizeof(Vertex));
        }
        // Reading faces in ascii format
        for (int i = 0; i < header.faceCount; i++) {
            if (!std::getline(plyFile, line)) {
                std::cerr << "Error: Failed to read faces." << std::endl;
                return;
            }
            std::istringstream iss(line);
            int count;
            iss >> count;
            std::vector<int> indices(count);
            for (int j = 0; j < count; j++) {
                iss >> indices[j];
            }
            // Note: we add 1 to convert from 0-based to 1-based index
            if (count == 3) {
                TriangleIndices tri { indices[0] + 1, indices[1] + 1, indices[2] + 1 };
                faceOut.write(reinterpret_cast<const char*>(&tri), sizeof(TriangleIndices));
            } else if (count > 3) {
                // Fan triangulation for polygons with more than 3 vertices
                for (int j = 1; j < count - 1; j++) {
                    TriangleIndices tri { indices[0] + 1, indices[j] + 1, indices[j+1] + 1 };
                    faceOut.write(reinterpret_cast<const char*>(&tri), sizeof(TriangleIndices));
                }
            }
        }
    } else {
        // For the binary format, we assume that vertices are stored as 3 floats (x, y, z)
        for (int i = 0; i < header.vertexCount; i++) {
            float x, y, z;
            plyFile.read(reinterpret_cast<char*>(&x), sizeof(float));
            plyFile.read(reinterpret_cast<char*>(&y), sizeof(float));
            plyFile.read(reinterpret_cast<char*>(&z), sizeof(float));
            Vertex v { static_cast<double>(x), static_cast<double>(y), static_cast<double>(z) };
            vertexOut.write(reinterpret_cast<const char*>(&v), sizeof(Vertex));
        }
        // Reading faces in binary :
        // We assume that each face is stored in the form: [uchar count][int indices...]
        for (int i = 0; i < header.faceCount; i++) {
            unsigned char count;
            plyFile.read(reinterpret_cast<char*>(&count), sizeof(unsigned char));
            std::vector<int> indices(count);
            for (int j = 0; j < count; j++) {
                int idx;
                plyFile.read(reinterpret_cast<char*>(&idx), sizeof(int));
                indices[j] = idx;
            }
            if (count == 3) {
                TriangleIndices tri { indices[0] + 1, indices[1] + 1, indices[2] + 1 };
                faceOut.write(reinterpret_cast<const char*>(&tri), sizeof(TriangleIndices));
            } else if (count > 3) {
                for (int j = 1; j < count - 1; j++) {
                    TriangleIndices tri { indices[0] + 1, indices[j] + 1, indices[j+1] + 1 };
                    faceOut.write(reinterpret_cast<const char*>(&tri), sizeof(TriangleIndices));
                }
            }
        }
    }
    vertexOut.close();
    faceOut.close();
    plyFile.close();
}

void convertPLYtoOBJSoup(const std::string &plyFilename, const std::string &outputFilename,
                         const std::string &tempVertexFile, const std::string &tempTriangleIndicesFile) {
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    // Converting PLY files into temporary binary files
    processPLYFile(plyFilename, tempVertexFile, tempTriangleIndicesFile);

    // Three dereferencing passes (identical to OBJ conversion)
    dereferencePass1(tempTriangleIndicesFile, tempVertexFile, "triangles_pass1.bin");
    dereferencePass2("triangles_pass1.bin", tempVertexFile, "triangles_pass2.bin");
    dereferencePass3("triangles_pass2.bin", tempVertexFile, "triangles_final.bin");

    // Writing the final OBJSoup file (text format)
    std::ifstream finalTriangles("triangles_final.bin", std::ios::binary);
    std::ofstream outFile(outputFilename);
    if (!finalTriangles.is_open() || !outFile.is_open()) {
        std::cerr << "Erreur : impossible d'ouvrir le fichier final." << std::endl;
        return;
    }
    TriangleCoordinates tri{};
    while (finalTriangles.read(reinterpret_cast<char*>(&tri), sizeof(TriangleCoordinates))) {
        outFile << "f " << tri.v1.toString() << " "
                << tri.v2.toString() << " "
                << tri.v3.toString() << "\n";
    }
    finalTriangles.close();
    outFile.close();

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
              << "Output written to " << outputFilename << std::endl;

    // Deleting temporary files
    std::remove(tempTriangleIndicesFile.c_str());
    std::remove("triangles_pass1.bin");
    std::remove("triangles_pass2.bin");
    std::remove("triangles_final.bin");
}
