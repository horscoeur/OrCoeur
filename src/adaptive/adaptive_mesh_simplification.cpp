#include <fstream>

#include "adaptive/adaptive_mesh_simplification.h"

void resetAggregatedQuadric(BSPNode* node) {
    if (node == nullptr) return;

    // Reset the aggregated quadric for the current node
    node->aggregatedQuadric = Quadric();

    // Recursively reset the aggregated quadric for child nodes
    resetAggregatedQuadric(node->left);
    resetAggregatedQuadric(node->right);
}

void computeRepresentative(std::vector<Vertex>& representativeVertices, BSPNode* node) {
    if (node == nullptr) return;

    // Compute the representative for the current node
    node->representative = findOptimalVertex(node->aggregatedQuadric);
    representativeVertices.push_back(node->representative);

    // Recursively compute the representative for child nodes
    computeRepresentative(representativeVertices, node->left);
    computeRepresentative(representativeVertices, node->right);
}

void distributeQuadrics(std::ifstream& file, const std::streampos dataStart, BSPNode* root) {
    file.clear();
    file.seekg(dataStart);

    TriangleCoordinates face{};
    while (file.read(reinterpret_cast<char*>(&face), sizeof(TriangleCoordinates))) {
        // Retrieve the vertices
        Vertex v1 = face.v1;
        Vertex v2 = face.v2;
        Vertex v3 = face.v3;

        // Traverse the BSP tree to find the leaf containing each vertex
        BSPNode* node1 = root->traverse(v1);
        BSPNode* node2 = root->traverse(v2);
        BSPNode* node3 = root->traverse(v3);

        // Compute the quadric of the face
        Quadric q = computeQuadric(face);

        // Distribute the quadric to the leaves containing the vertices
        node1->aggregatedQuadric = addQuadric(node1->aggregatedQuadric, q);
        node2->aggregatedQuadric = addQuadric(node2->aggregatedQuadric, q);
        node3->aggregatedQuadric = addQuadric(node3->aggregatedQuadric, q);
    }
}

int writeSimplifiedMesh(std::ifstream& file, std::ofstream& outputFile, const std::streampos dataStart, BSPNode* root) {
    file.clear();
    file.seekg(dataStart);

    TriangleCoordinates face{};
    int faceCount = 0;

    while (file.read(reinterpret_cast<char*>(&face), sizeof(TriangleCoordinates))) {
        Vertex v1 = face.v1;
        Vertex v2 = face.v2;
        Vertex v3 = face.v3;

        // Traverse the BSP tree to find the leaf containing each vertex
        const BSPNode* node1 = root->traverse(v1);
        const BSPNode* node2 = root->traverse(v2);
        const BSPNode* node3 = root->traverse(v3);

        // Discard degenerate faces (all vertices map to the same or repeated leaves)
        if (node1 == node2 || node2 == node3 || node1 == node3) {
            continue;
        }

        // Write the face to the output file with representatives as vertices
        face.v1 = node1->representative;
        face.v2 = node2->representative;
        face.v3 = node3->representative;

        outputFile.write(reinterpret_cast<char*>(&face), sizeof(TriangleCoordinates));
        faceCount++;
    }

    return faceCount;
}


void adaptiveMeshSimplification(const std::string &inputFacesFilenameBinary, const std::string &outputFilenameBinary, BSPNode* root) {
    // Open the input file
    std::ifstream file(inputFacesFilenameBinary, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << inputFacesFilenameBinary << ".\n";
        return;
    }

    // Open the output file
    std::ofstream outputFile(outputFilenameBinary, std::ios::binary);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output file " << outputFilenameBinary << ".\n";
        file.close();
        return;
    }

    // Skip header lines
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("END_HEADER") != std::string::npos) break;
    }

    // Get the end-of-header position
    std::streampos dataStart = file.tellg();
    if (dataStart == -1) {
        std::cerr << "Error: Could not find the start of face data.\n";
        return;
    }

    resetAggregatedQuadric(root);
    distributeQuadrics(file, dataStart, root);

    // Iterate over the BSP tree to compute the representative of each leaf
    std::vector<Vertex> representativeVertices;
    computeRepresentative(representativeVertices, root);

    // Write header and remember position of face_count
    outputFile << "format: binary_little_endian\n";
    outputFile << "vertex_count: " << representativeVertices.size() << "\n";

    std::streampos faceCountPos = outputFile.tellp();
    outputFile << "face_count: 0000000000\n";
    outputFile << "END_HEADER\n";

    // Write faces and get actual count
    int faceCount = writeSimplifiedMesh(file, outputFile, dataStart, root);

    // Rewrite the correct face_count at the saved position
    outputFile.seekp(faceCountPos);
    outputFile << "face_count: " << std::setw(10) << std::setfill(' ') << faceCount << "\n";

    file.close();
    outputFile.close();
    std::cout << "Adaptive mesh simplification completed. Output saved to " << outputFilenameBinary << ".\n";
}
