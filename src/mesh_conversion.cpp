#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

#include "mesh_conversion.h"
#include "structures.h"
#include "utility.h"


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

    TriangleIndices face{};
    Vertex currentVertex{};
    int currentIndex = 1; // OBJ indices start at 1

    if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first vertex (pass1)." << std::endl;
        std::remove("sorted_by_v1.bin");
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&face), sizeof(TriangleIndices))) {
        // Advance in the vertices until the vertex corresponding to tri.v1 is found
        while (currentIndex < face.v1) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << face.v1 << " not found in the vertex file." << std::endl;
                std::remove("sorted_by_v1.bin");
                return;
            }
            currentIndex++;
        }
        // Write a Triangle_Pass1 record with the dereferenced v1
        Triangle_Pass1 rec{};
        rec.v1 = currentVertex;
        rec.v2 = face.v2;
        rec.v3 = face.v3;
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
        std::remove("sorted_by_v2.bin");
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&recIn), sizeof(Triangle_Pass1))) {
        while (currentIndex < recIn.v2) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << recIn.v2 << " not found (pass2)." << std::endl;
                std::remove("sorted_by_v2.bin");
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
        std::remove("sorted_by_v3.bin");
        return;
    }

    while (sortedTriangles.read(reinterpret_cast<char*>(&recIn), sizeof(Triangle_Pass2))) {
        while (currentIndex < recIn.v3) {
            if (!vertices.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                std::cerr << "Error: Index " << recIn.v3 << " not found (pass3)." << std::endl;
                std::remove("sorted_by_v3.bin");
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

// ------------------- Exporting the final OBJSoup file -------------------
// This function reads the final binary OBJSoup file and writes its contents to a text file.
void exportBinaryOBJSoupToText(const std::string &inputFilename, const std::string &outputFilename, int faceCount) {
    std::ifstream finalTriangles(inputFilename, std::ios::binary);
    std::ofstream outFile(outputFilename);

    if (!finalTriangles.is_open() || !outFile.is_open()) {
        std::cerr << "Error: Unable to open final file." << std::endl;
        return;
    }

    TriangleCoordinates face{};

    // Write the header
    outFile << "format: ascii\n";
    outFile << "face_count: " << faceCount << "\n";
    outFile << "END_HEADER\n";

    while (finalTriangles.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))) {
        outFile << "f " << face.v1.toString() << " " << face.v2.toString() << " " << face.v3.toString() << "\n";
    }

    finalTriangles.close();
    outFile.close();
}


// ------------------- Conversion from OBJ to OBJSoup -------------------
// It first writes temporary binary files for vertices and triangle indices,
// then performs the three dereferencing passes to obtain the final text file.
void convertOBJtoOBJSoup(const std::string &objFilename, const std::string &outputFilename) {
    auto start = std::chrono::high_resolution_clock::now();

    // Phase 1: Convert OBJ to two temporary binary files
    std::ifstream objFile(objFilename);
    std::ofstream vertexFile("vertices.bin", std::ios::binary);
    std::ofstream triangleIndicesFile("triangles.bin", std::ios::binary);

    if (!objFile.is_open() || !vertexFile.is_open() || !triangleIndicesFile.is_open()) {
        std::cerr << "Error: Unable to open temporary files." << std::endl;
        return;
    }

    // Phase 1: Convert OBJ to two temporary binary files
    std::string line;
    int faceCount = 0;
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
            faceCount++;
        }
    }
    objFile.close();
    vertexFile.close();
    triangleIndicesFile.close();

    // Phase 2: The 3 dereferencing passes
    // Pass 1: Dereference v1
    dereferencePass1("triangles.bin", "vertices.bin", "triangles_pass1.bin");
    // Pass 2: Dereference v2
    dereferencePass2("triangles_pass1.bin", "vertices.bin", "triangles_pass2.bin");
    // Pass 3: Dereference v3
    dereferencePass3("triangles_pass2.bin", "vertices.bin", "triangles_final.bin");

    // Remove the existing output file if it exists
    std::remove(outputFilename.c_str());

    // If outputFilename ends with ".bin", add a header (for the number of triangles and format) and move the binary file directly to the output
    // Else, convert the binary file to a well-formatted text file
    if (outputFilename.substr(outputFilename.size() - 4) == ".bin") {
        std::ofstream out(outputFilename, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: Unable to open output file." << std::endl;
            return;
        }

        // Write the header in ascii format
        out << "format: binary_little_endian\n";
        out << "face_count: " << faceCount << "\n";
        out << "END_HEADER\n";

        std::ifstream finalTriangles("triangles_final.bin", std::ios::binary);
        out << finalTriangles.rdbuf();
        finalTriangles.close();
        out.close();
    } else {
        exportBinaryOBJSoupToText("triangles_final.bin", outputFilename, faceCount);
    }

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
              << "Output written to " << outputFilename << std::endl;

    // Delete temporary files
    std::remove("vertices.bin");
    std::remove("triangles.bin");
    std::remove("triangles_pass1.bin");
    std::remove("triangles_pass2.bin");
    std::remove("triangles_final.bin");
}


// ------------------- Conversion from PLY to OBJSoup -------------------
void convertPLYtoOBJSoup(const std::string &plyFilename, const std::string &outputFilename) {
    const std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    // Converting PLY files into temporary binary files
    const int faceCount = processPLYFile(plyFilename, "vertices.bin", "triangles.bin");

    // Three dereferencing passes (identical to OBJ conversion)
    dereferencePass1("triangles.bin", "vertices.bin", "triangles_pass1.bin");
    dereferencePass2("triangles_pass1.bin", "vertices.bin", "triangles_pass2.bin");
    dereferencePass3("triangles_pass2.bin", "vertices.bin", "triangles_final.bin");

    // Remove the existing output file if it exists
    std::remove(outputFilename.c_str());

    // If outputFilename ends with ".bin", move the binary file directly to the output
    // Else, convert the binary file to a well-formatted text file
    if (outputFilename.substr(outputFilename.size() - 4) == ".bin") {
        std::ofstream out(outputFilename, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Error: Unable to open output file." << std::endl;
            return;
        }

        // Write the header in ascii format
        out << "format: binary_little_endian\n";
        out << "face_count: " << faceCount << "\n";
        out << "END_HEADER\n";

        std::ifstream finalTriangles("triangles_final.bin", std::ios::binary);
        out << finalTriangles.rdbuf();
        finalTriangles.close();
        out.close();
    } else {
        exportBinaryOBJSoupToText("triangles_final.bin", outputFilename, faceCount);
    }

    const std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
              << "Output written to " << outputFilename << std::endl;

    // Deleting temporary files
    std::remove("triangles.bin");
    std::remove("vertices.bin");
    std::remove("triangles_pass1.bin");
    std::remove("triangles_pass2.bin");
    std::remove("triangles_final.bin");
}

// Parse the header of an OBJSoup file to extract the format and number of faces.
ORCOEURHeader parseORCOEURHeader(std::ifstream &file) {
    std::string line;
    std::string format;
    int faceCount;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line == "END_HEADER") break;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "format:") {
            iss >> format;
        } else if (key == "face_count:") {
            iss >> faceCount;
        }
    }

    return {format, faceCount};
}


// Read the ASCII file and write the vertices and faces to separate text files.
void processASCII(std::ifstream &file, std::ofstream &vertexFile, std::ofstream &faceFile) {
    std::string line;
    int nextIndex = 1;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        std::string type;
        iss >> type;

        if (type == "f") {
            Vertex v1{}, v2{}, v3{};
            iss >> v1.x >> v1.y >> v1.z >> v2.x >> v2.y >> v2.z >> v3.x >> v3.y >> v3.z;

            // Writing faces
            vertexFile << "v " << v1.toString() << "\n";
            vertexFile << "v " << v2.toString() << "\n";
            vertexFile << "v " << v3.toString() << "\n";

            // Writing vertices
            faceFile << "f " << nextIndex << " " << (nextIndex + 1) << " " << (nextIndex + 2) << "\n";
            nextIndex += 3;
        }
    }
}

// Read the binary file and write the vertices and faces to separate text files.
void processBinary(std::ifstream &file, std::ofstream &vertexFile, std::ofstream &faceFile) {
    int nextIndex = 1;
    TriangleCoordinates face{};

    while (file.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))) {
        vertexFile << "v " << face.v1.toString() << "\n";
        vertexFile << "v " << face.v2.toString() << "\n";
        vertexFile << "v " << face.v3.toString() << "\n";

        faceFile << "f " << nextIndex << " " << (nextIndex + 1) << " " << (nextIndex + 2) << "\n";
        nextIndex += 3;
    }
}


// ------------------- Conversion from OBJSoup to OBJ -------------------
void convertOBJSoupToOBJ(const std::string &inputFilename, const std::string &outputFilename) {
    std::ifstream objFile(inputFilename, std::ios::binary);
    if (!objFile.is_open()) {
        std::cerr << "Error: Unable to open input file." << std::endl;
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Parsing the header to determine the format and number of faces
    auto [format, faceCount] = parseORCOEURHeader(objFile);
    if (format.empty()) {
        std::cerr << "Error: Invalid or missing format in the header." << std::endl;
        return;
    }

    // Creating temporary files for vertices and faces
    std::ofstream vertexFile("vertices.bin", std::ios::binary);
    std::ofstream faceFile("triangles.bin", std::ios::binary);
    if (!vertexFile.is_open() || !faceFile.is_open()) {
        std::cerr << "Error: Unable to create temporary files." << std::endl;
        return;
    }

    if (format == "ascii") {
        processASCII(objFile, vertexFile, faceFile);
    } else if (format == "binary_little_endian") {
        processBinary(objFile, vertexFile, faceFile);
    } else {
        std::cerr << "Error: Unsupported format: " << format << std::endl;
        return;
    }

    objFile.close();
    vertexFile.close();
    faceFile.close();

    // Remove the existing output file if it exists
    std::remove(outputFilename.c_str());

    // Concatenating the temporary files into the final OBJ file
    std::ofstream finalOutFile(outputFilename);
    std::ifstream vertexFileIn("vertices.bin");
    std::ifstream faceFileIn("triangles.bin");

    if (!vertexFileIn.is_open() || !faceFileIn.is_open() || !finalOutFile.is_open()) {
        std::cerr << "Error: Unable to open intermediate files." << std::endl;
        return;
    }

    finalOutFile << "# OrCoeur - Converted OBJ file\n";
    finalOutFile << vertexFileIn.rdbuf();
    finalOutFile << faceFileIn.rdbuf();

    vertexFileIn.close();
    faceFileIn.close();
    finalOutFile.close();

    // Deleting temporary files
    std::remove("vertices.bin");
    std::remove("triangles.bin");

    std::chrono::duration<double> duration = std::chrono::high_resolution_clock::now() - start;
    std::cout << "Conversion completed in " << duration.count() << " seconds. "
              << "Output written to " << outputFilename << std::endl;
}


// Parse the header of a PLY file to extract the number of vertices and faces.
bool parsePLYHeader(std::istream &in, PLYHeader &header) {
    std::string line;
    if (!std::getline(in, line) || line != "ply") {
        std::cerr << "Error: The file must start with the 'ply' keyword." << std::endl;
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


// Process the PLY file and write the vertices and faces to temporary binary files.
int processPLYFile(const std::string &plyFilename, const std::string &tempVertexFile,
                   const std::string &tempTriangleIndicesFile) {
    // Open in binary mode to read the header (which is in ascii)
    std::ifstream plyFile(plyFilename, std::ios::in | std::ios::binary);
    if (!plyFile.is_open()) {
        std::cerr << "Error: Unable to open " << plyFilename << std::endl;
        return -1;
    }
    PLYHeader header;
    if (!parsePLYHeader(plyFile, header)) {
        std::cerr << "Error: Invalid PLY header." << std::endl;
        return -1;
    }
    bool isAscii = (header.format == "ascii");

    std::ofstream vertexOut(tempVertexFile, std::ios::binary);
    std::ofstream faceOut(tempTriangleIndicesFile, std::ios::binary);
    if (!vertexOut.is_open() || !faceOut.is_open()) {
        std::cerr << "Error: Unable to open temporary files." << std::endl;
        return -1;
    }

    if (isAscii) {
        // Reading vertices in ascii format
        std::string line;
        for (int i = 0; i < header.vertexCount; i++) {
            if (!std::getline(plyFile, line)) {
                std::cerr << "Error: Failed to read vertices." << std::endl;
                return -1;
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
                return -1;
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

            // If the file is big-endian, we need to swap the bytes
            if (header.format == "binary_big_endian") {
                x = swapFloat(x);
                y = swapFloat(y);
                z = swapFloat(z);
            }

            Vertex v { x, y, z };
            vertexOut.write(reinterpret_cast<const char*>(&v), sizeof(Vertex));
        }
        // Reading faces in binary :
        // We assume that each face is stored in the form: [uchar count][int indices...]
        for (int i = 0; i < header.faceCount; i++) {
            unsigned char count;
            plyFile.read(reinterpret_cast<char*>(&count), sizeof(unsigned char));
            std::vector<int> indices(count);
            for (int j = 0; j < count; j++) {
                int index;
                plyFile.read(reinterpret_cast<char*>(&index), sizeof(int));

                // If the file is big-endian, we need to swap the bytes
                if (header.format == "binary_big_endian") {
                    index = static_cast<int>(swapUInt32(static_cast<uint32_t>(index)));
                }

                indices[j] = index;
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

    return header.faceCount;
}