#include "dereference_passes.h"
#include "structures.h"
#include "utility.h"
#include "mesh_conversion.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>


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
            std::vector<int> indices;
            std::string token;

            // Extract all vertex indices of the face
            while (iss >> token) {
                int idx = std::stoi(token.substr(0, token.find('/')));
                indices.push_back(idx);
            }

            // Fan triangulation: for a face with n vertices, create n-2 triangles
            for (size_t i = 1; i < indices.size() - 1; ++i) {
                TriangleIndices t{};
                t.v1 = indices[0];      // first vertex of the face
                t.v2 = indices[i];      // current vertex
                t.v3 = indices[i + 1];  // next vertex
                triangleIndicesFile.write(reinterpret_cast<const char *>(&t), sizeof(TriangleIndices));
                faceCount++;
            }
        }
    }
    objFile.close();
    vertexFile.close();
    triangleIndicesFile.close();

    // Phase 2: The 3 dereferencing passes
    dereferencePass1("triangles.bin", "vertices.bin", "triangles_pass1.bin");
    dereferencePass2("triangles_pass1.bin", "vertices.bin", "triangles_pass2.bin");
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