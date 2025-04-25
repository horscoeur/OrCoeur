#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unistd.h>

#include "stream_simplification.h"
#include <polyscope/surface_mesh.h>
#include <mesh_loader.h>

#include "quadrics.h"


void writeHeader(std::ofstream& outputFile, int faceCount, bool isBinary = true) {
    if (!outputFile.is_open()) {
        std::cerr << "Error: Output file not open for writing header." << std::endl;
        return;
    }

    if (isBinary) {
        outputFile << "format: binary_little_endian" << std::endl;
    }
    else {
        outputFile << "format: ascii" << std::endl;
    }

    outputFile << "face_count: " << faceCount << std::endl;
    outputFile << "END_HEADER" << std::endl;
}

int getTriangleCount(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " for vertex counting.\n";
        return -1;
    }

    int faceCount = 0;
    std::string line;

    while (std::getline(file, line)) {
        if (line == "END_HEADER")
            break;

        if (line.substr(0, 11) == "face_count:") {
            std::istringstream iss(line);
            std::string dummy;
            iss >> dummy >> faceCount;
        }
    }

    file.close();

    return faceCount;
}

void displayFromInputFile(const std::string& inputFilePath, int startIndex, int endIndex) {
    std::vector<std::array<float, 3>> unReadVertices;
    std::vector<std::array<int, 3>> unReadFaces;

    std::cout << "Extract vertices from : " << inputFilePath << std::endl;

    std::vector<TriangleCoordinates> unReadTriangles = extractTrianglesFromBinary(inputFilePath, startIndex, endIndex);

    std::cout << "Triangles extracted FROM INPUT : " << unReadTriangles.size() << std::endl;
    if (unReadTriangles.size() != 0) {
        for (int i = 0; i < unReadTriangles.size(); i++) {
            unReadVertices.push_back({unReadTriangles[i].v1.x, unReadTriangles[i].v1.y, unReadTriangles[i].v1.z});
            unReadVertices.push_back({unReadTriangles[i].v2.x, unReadTriangles[i].v2.y, unReadTriangles[i].v2.z});
            unReadVertices.push_back({unReadTriangles[i].v3.x, unReadTriangles[i].v3.y, unReadTriangles[i].v3.z});
            unReadFaces.push_back({3 * i, 3 * i + 1, 3 * i + 2});
        }

        polyscope::registerSurfaceMesh("Unread Mesh", unReadVertices, unReadFaces);
        auto meshPtr3 = polyscope::getSurfaceMesh("Unread Mesh");
        meshPtr3->setSurfaceColor(glm::vec3(0.2, 0.2, 0.8)); // blue
    }
}

void displayFromOutputFile(const std::string& outputFilePath, int startIndex, int endIndex) {
    std::vector<std::array<float, 3>> writtenVertices;
    std::vector<std::array<int, 3>> writtenFaces;

    std::cout << "Extract vertices from : " << outputFilePath << std::endl;

    std::vector<TriangleCoordinates> writtenTriangles =
        extractTrianglesFromBinary(outputFilePath, startIndex, endIndex);

    std::cout << "Triangles extracted FROM OUTPUT: " << writtenTriangles.size() << std::endl;
    if (writtenTriangles.size() != 0) {
        for (int i = 0; i < writtenTriangles.size(); i++) {
            writtenVertices.push_back({writtenTriangles[i].v1.x, writtenTriangles[i].v1.y, writtenTriangles[i].v1.z});
            writtenVertices.push_back({writtenTriangles[i].v2.x, writtenTriangles[i].v2.y, writtenTriangles[i].v2.z});
            writtenVertices.push_back({writtenTriangles[i].v3.x, writtenTriangles[i].v3.y, writtenTriangles[i].v3.z});
            writtenFaces.push_back({3 * i, 3 * i + 1, 3 * i + 2});
        }

        polyscope::registerSurfaceMesh("Out of core mesh", writtenVertices, writtenFaces);
        auto meshPtr2 = polyscope::getSurfaceMesh("Out of core mesh");
        meshPtr2->setSurfaceColor(glm::vec3(0.2, 0.8, 0.2));
    }
}

void displayFromBuffer(std::map<int, TriangleCoordinates>& inCoreTriangleBuffer) {
    std::vector<std::array<float, 3>> inCoreVertices;
    std::vector<std::array<int, 3>> inCoreFaces;

    for (int i = 0; i < inCoreTriangleBuffer.size(); i++) {
        inCoreVertices.push_back({ inCoreTriangleBuffer[i].v1.x, inCoreTriangleBuffer[i].v1.y, inCoreTriangleBuffer[i].v1.z });
        inCoreVertices.push_back({ inCoreTriangleBuffer[i].v2.x, inCoreTriangleBuffer[i].v2.y, inCoreTriangleBuffer[i].v2.z });
        inCoreVertices.push_back({ inCoreTriangleBuffer[i].v3.x, inCoreTriangleBuffer[i].v3.y, inCoreTriangleBuffer[i].v3.z });
        inCoreFaces.push_back({3 * i, 3 * i + 1, 3 * i + 2});
    }

    polyscope::registerSurfaceMesh("In core mesh", inCoreVertices, inCoreFaces);
    auto meshPtr2 = polyscope::getSurfaceMesh("In core mesh");
    meshPtr2->setSurfaceColor(glm::vec3(0.8, 0.2, 0.2));
}

bool streamSimplificationVisualization(const std::string& inputFileName, const std::string& outputFileName,
                                       int maxTrianglesInBuffer, float decimationPercentage,
                                       bool visualizeSimplification) {
    std::ifstream inputFile(inputFileName, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cerr << "Error: Could not open file " << inputFileName << ".\n";
        return false;
    }
    else {
        std::cout << "Input file " << inputFileName << " oppened" << ".\n";
    }

    std::ofstream outputFile(outputFileName, std::ios::binary | std::ios::app);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open file " << outputFileName << ".\n";
        return false;
    }
    else {
        std::cout << "Output file " << outputFileName << " oppened" << ".\n";
    }

    int nbTrianglesRead = 0;
    int nbTrianglesWritten = 0;
    int nbTriangleTotal = getTriangleCount(inputFileName);

    writeHeader(outputFile, nbTriangleTotal, true);


    StreamMeshData meshData = StreamMeshData();

    // modif la facon dont les triangles sont trié dans le bianire en prennant le barycentre
    initBuffer(inputFile, maxTrianglesInBuffer * 0.5, decimationPercentage,meshData);
    // float R = trianglesInCore / trianglesRead - (trianglesWritten/decimationPercentage);

    std::cout << "======INIT======" << std::endl;
    std::cout << "Read : " << nbTrianglesRead << std::endl;
    std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;
    std::cout << "======START LOOP======" << std::endl;


    while (!inputFile.eof()) {
        read(inputFile, maxTrianglesInBuffer * 0.5, meshData);
        std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;
        std::cout << "Written  : " << nbTrianglesWritten << std::endl;

        // decimate ((1-decimationPercentage) * maxTrianglesInBuffer / 4)

        if (visualizeSimplification) {
            displayFromOutputFile(outputFileName, 0, nbTrianglesWritten);
            displayFromBuffer(meshData.inCoreTriangleBuffer);
            if (nbTrianglesWritten + meshData.inCoreTriangleBuffer.size() < nbTriangleTotal) {
                displayFromInputFile(inputFileName, nbTrianglesWritten + meshData.inCoreTriangleBuffer.size(), nbTriangleTotal);
            }
            polyscope::show();
        }
        if (meshData.inCoreTriangleBuffer.size() == nbTriangleTotal) {
            std::cout << "All triangles read, exiting loop." << std::endl;
        }
        std::cout << "====== Je suis à la fin de cette boucle ======" << std::endl;
        std::cout << "L'etat de input file est de " << inputFile.eof() << std::endl;
        // normalement write(outputFile, inCoreTriangleBuffer, maxTrianglesInBuffer * 0.5, &nbTrianglesWritten, &nbTrianglesInCore);
        //write(outputFile, inCoreTriangleBuffer, nbTrianglesInCore, &nbTrianglesWritten, &nbTrianglesInCore);
    }
    decimate(decimationPercentage, meshData);

    // ici quand on vide le buffer, il faut faire un truc (voir papier)
    if (meshData.inCoreTriangleBuffer.size() > 0) {
        write(outputFile, meshData.inCoreTriangleBuffer,maxTrianglesInBuffer * 0.5 , &nbTrianglesWritten);
    }

    if (visualizeSimplification) {
        displayFromOutputFile(outputFileName, 0, nbTrianglesWritten);
        polyscope::removeSurfaceMesh("Unread Mesh");
        polyscope::removeSurfaceMesh("In core mesh");
        polyscope::show();
    }


    return true;
}

// alterne reading / decimations until R == p
bool initBuffer(std::ifstream& inputFile, int numberToRead, float decimationPercentage,
                StreamMeshData& meshData) {
    // skip le header
    std::string line;
    while (std::getline(inputFile, line)) {
        if (line == "END_HEADER")
            break;
    }

    int n = 1.0f / decimationPercentage;
    std::cout << " n : " << n << std::endl;
    read(inputFile, numberToRead, meshData);

    // for (int i=0; i < n; i++) {
    // inputFile.read(reinterpret_cast<char*>(inCoreTriangleBuffer.data()), (maxTrianglesInBuffer/2) * sizeof(TriangleCoordinates));
    // decimate(inCoreTriangleBuffer, maxTrianglesInBuffer/4);
    //}

    // apres init buffer on devrait avoir lu au total n/2 n/2 Nmax triangles et ecrit 0
    return true;
}


void updateAdjacency(int vertexId, int neighborId, StreamMeshData& meshData) {
    auto& adj = meshData.adjacencyList[vertexId];
    auto itExisting = std::find(adj.begin(), adj.end(), neighborId);

    if (itExisting == adj.end()) {
        // if the neighbor is not already in the adjacency list, add it
        auto itInsert = std::lower_bound(adj.begin(), adj.end(), neighborId);
        adj.insert(itInsert, neighborId);
    }
    else {
        // if the neighbor is already in the adjacency list, add it to the adjacency list already seen and remove it from the adjacency list
        adj.erase(std::remove(adj.begin(), adj.end(), neighborId), adj.end());
        if (adj.empty()) {
            meshData.vertexNotInBorder.push_back(vertexId);
        }
        auto& v = meshData.adjacencyListAlreadySeen[vertexId];
        // ensure the adjacency list already seen is sorted
        auto it = std::lower_bound(v.begin(), v.end(), neighborId);
        if (it == v.end() || *it != neighborId)
            v.insert(it, neighborId);

    }
}

bool read(std::ifstream& inputFile,int numberToRead, StreamMeshData& meshData) {
    std::cout << "Reading " << numberToRead << " Faces from inputfile\n";

    TriangleCoordinates triangle;
    for (int i = 0; i < numberToRead; ++i) {
        if (!inputFile.read(reinterpret_cast<char*>(&triangle), sizeof(triangle)))
            break; // End of file or reading error

        // Storage the triangle in the inCoreTriangleBuffer
        meshData.inCoreTriangleBuffer[meshData.unique_triangle_index] = triangle;
        // Compute the quadric for the triangle
        Quadric q = computeQuadric(triangle);

        // ensureVertex is a lambda function that ensures the vertex is in the vertexMap and add it if not
        auto ensureVertex = [&](const Vertex& v) {
            if (meshData.vertexMap.find(v) == meshData.vertexMap.end()) {
                meshData.vertexMap[v] = meshData.actual_unique_id++;
            }
            return meshData.vertexMap[v];
        };

        int id1 = ensureVertex(triangle.v1);
        int id2 = ensureVertex(triangle.v2);
        int id3 = ensureVertex(triangle.v3);

        // process is a lambda function that processes the triangle and updates the triangleList and triangleQuadricMap
        auto process = [&](int vid, int nid1, int nid2) {
            meshData.triangleList[vid].push_back(meshData.unique_triangle_index);
            if (meshData.triangleQuadricMap.find(vid) == meshData.triangleQuadricMap.end()) {
                meshData.triangleQuadricMap[vid] = q;
            }
            else {
                meshData.triangleQuadricMap[vid] = addQuadric(meshData.triangleQuadricMap[vid], q);
            }
            updateAdjacency(vid, nid1, meshData);
            updateAdjacency(vid, nid2, meshData);
        };

        process(id1, id2, id3);
        process(id2, id1, id3);
        process(id3, id1, id2);

        meshData.unique_triangle_index++;
    }
    return true;
}

// DEPRECATED FOR NOW
bool write(std::ofstream& outputFile, std::map<int, TriangleCoordinates>& inCoreTriangleBuffer, int numberToWrite,
           int* trianglesWritten) {
    // ajouter le truc pour choisir aléatoirement puis prendre le trianqle avec la plus grande erreur ?
    std::cout << "Writting " << numberToWrite << " Faces to outputfile " << std::endl;

    // la j'ecrit tout ce que qu'il y a dans le buffer pour le test de visualisation
    //outputFile.write(reinterpret_cast<char*>(inCoreTriangleBuffer.data()), numberToWrite * sizeof(TriangleCoordinates));

    //inCoreTriangleBuffer.clear();
    *trianglesWritten += numberToWrite;

    return true;
}


void decimatePartAdjacencyListAlreadySeen(StreamMeshData& meshData, int vertexA, int vertexB,
                                          int newIndice, int oldIndice) {
    // Get the neighbors of vertexA and vertexB
    std::vector<int>& neighborsVertexB = meshData.adjacencyListAlreadySeen[vertexB];
    std::vector<int>& neighborsVertexA = meshData.adjacencyListAlreadySeen[vertexA];
    // Initialize the new adjacency list for the new vertex
    std::vector<int> neighborsToMyNewIndice = std::vector<int>();

    // Get the size of the lists
    size_t sizeNeighborToRemove = neighborsVertexB.size();
    size_t sizeVertexToRemove = neighborsVertexA.size();
    size_t i = 0, j = 0;
    // Loop through the neighbors of vertexA and vertexB to update them and delete the common neighbors with the common triangle
    while (i < sizeNeighborToRemove && j < sizeVertexToRemove) {
        // If a common neighbor is found
        if (neighborsVertexB[i] == neighborsVertexA[j]) {
            // List of neighbors of the common neighbor of vertexA and vertexB
            int k = 0;
            std::vector<int> &listOfOurNeighBors = meshData.adjacencyListAlreadySeen[neighborsVertexB[i]];
            // Remove the vertexA and vertexB from the list of neighbors of the common neighbor
            while (k < listOfOurNeighBors.size()) {
                if (listOfOurNeighBors[k] == vertexB|| listOfOurNeighBors[k] == vertexA) {
                    listOfOurNeighBors.erase(listOfOurNeighBors.begin() + k);
                }
                else {
                    ++k;
                }
            }

            // -------------------------------Gestion of the degenerate triangle------------------------------
            // Get the triangle list of vertexA, vertexB and the common neighbor
            std::vector<int>& triangleOfVertexA = meshData.triangleList[vertexA];
            std::vector<int>& triangleOfVertexB = meshData.triangleList[vertexB];
            std::vector<int>& triangleOfVertexC = meshData.triangleList[neighborsVertexB[i]];
            size_t a = 0,b = 0, c = 0;
            // Loop through the triangle lists to find the common triangle
            // Warning : the triangle lists are sorted in ascending order
            while (a < triangleOfVertexA.size() && b < triangleOfVertexB.size() &&  c < triangleOfVertexC.size()) {
                // If the common triangle is found, remove it from the triangle list of vertexA, vertexB and vertexC
                if (triangleOfVertexB[b] == triangleOfVertexA[a] && triangleOfVertexA[a] == triangleOfVertexC[c]) {
                    meshData.inCoreTriangleBuffer.erase(triangleOfVertexB[b]);
                    triangleOfVertexB.erase(triangleOfVertexB.begin() + b);
                    triangleOfVertexA.erase(triangleOfVertexA.begin() + a);
                    triangleOfVertexC.erase(triangleOfVertexC.begin() + c);
                    break;
                }
                else {
                    // If the common triangle is not found, find the minimum value of the three triangle lists and increment the corresponding index
                    int minVal = std::min({ triangleOfVertexB[b], triangleOfVertexA[a], triangleOfVertexC[c] });
                    if (triangleOfVertexA[a] == minVal) ++a;
                    if (triangleOfVertexB[b] == minVal) ++b;
                    if (triangleOfVertexC[c] == minVal) ++c;
                }
            }
            // -----------------------------------------------------------------------------------------------

            ++i;
            ++j;
        }
        // If the neighbor of vertexB is smaller than the neighbor of vertexA and not equal to vertexA
        else if (neighborsVertexB[i] < neighborsVertexA[j]) {
            if (neighborsVertexB[i] != vertexA) {
                // List of neighbors of a neighbor of vertexB
                std::vector<int>& listOfMyNeighBors = meshData.adjacencyListAlreadySeen[neighborsVertexB[i]];
                // Loop through the list of neighbors of a neighbor of vertexB
                for (int k = 0; k < listOfMyNeighBors.size(); k++) {
                    // If the neighbor of vertexB is equal to vertexB, replace it with newIndice
                    if (listOfMyNeighBors[k] == vertexB) {
                        listOfMyNeighBors[k] = newIndice;
                    }
                }
                // Add the neighbor of vertexB to the list of neighbors of my newIndice
                neighborsToMyNewIndice.push_back(neighborsVertexB[i]);
            }
            ++i;
        }
        // If the neighbor of vertexA is smaller than the neighbor of vertexB and not equal to vertexB
        else {
            if (neighborsVertexA[j] != vertexB) {
                // List of neighbors of a neighbor of vertexA
                std::vector<int>& listOfMyNeighBors = meshData.adjacencyListAlreadySeen[neighborsVertexA[j]];
                // Loop through the list of neighbors of a neighbor of vertexA
                for (int k = 0; k < listOfMyNeighBors.size(); k++) {
                    // If the neighbor of vertexA is equal to vertexA, replace it with newIndice
                    if (listOfMyNeighBors[k] == vertexA) {
                        listOfMyNeighBors[k] = newIndice;
                    }
                }
                // Add the neighbor of vertexA to the list of neighbors of my newIndice
                neighborsToMyNewIndice.push_back(neighborsVertexA[j]);
            }
            ++j;
        }
    }

    // Loop through the remaining neighbors of vertexB:
    while (i < neighborsVertexB.size()) {
        // List of neighbors of a neighbor of vertexB
        std::vector<int>& listOfMyNeighBors = meshData.adjacencyListAlreadySeen[neighborsVertexB[i]];
        // Loop through the list of neighbors of a neighbor of vertexB
        for (int k = 0; k < listOfMyNeighBors.size(); k++) {
            // If the neighbor of vertexB is equal to vertexB, replace it with newIndice
            if (listOfMyNeighBors[k] == vertexB) {
                listOfMyNeighBors[k] = newIndice;
            }
        }
        // Add the neighbor of vertexB to the list of neighbors of my newIndice
        neighborsToMyNewIndice.push_back(neighborsVertexB[i]);
        ++i;
    }
    // Loop through the remaining neighbors of vertexA:
    while (j < neighborsVertexA.size()) {
        // List of neighbors of a neighbor of vertexA
        std::vector<int>& listOfMyNeighBors = meshData.adjacencyListAlreadySeen[neighborsVertexA[j]];
        // Loop through the list of neighbors of a neighbor of vertexA
        for (int k = 0; k < listOfMyNeighBors.size(); k++) {
            // If the neighbor of vertexA is equal to vertexA, replace it with newIndice
            if (listOfMyNeighBors[k] == vertexA) {
                listOfMyNeighBors[k] = newIndice;
            }
        }
        // Add the neighbor of vertexA to the list of neighbors of my newIndice
        neighborsToMyNewIndice.push_back(neighborsVertexA[j]);
        ++j;
    }
    // Remove the oldIndice from the adjacency list and keep the newIndice
    meshData.adjacencyListAlreadySeen.erase(oldIndice);
    meshData.adjacencyListAlreadySeen[newIndice] = neighborsToMyNewIndice;
}

void findCoordInVertexMap(StreamMeshData& meshData, int vertexA, int vertexB, int newIndice, Vertex& coordVertexA, Vertex& coordVertexB, Vertex& coordNewIndice) {
    // For now I am going to take the coordinates of the vertexToRemove and neighborToRemove from the vertexMap
    // TODO : Calculate the true coordinates of the newIndice thanks to the quadric (not implemented yet)
    // So for now I am just going to take the coordinate of the newIndice from the vertexMap
    bool foundVertexA = false;
    bool foundVertexB = false;
    bool foundVertexNewIndice = false; // TODO : Delete this variable
    for (const auto& [key, val] : meshData.vertexMap) {
        if (val == vertexA) {
            coordVertexA = key;
            foundVertexA = true;
        }
        if (val == vertexB) {
            coordVertexB = key;
            foundVertexB = true;
        }
        // TODO : Delete this condition
        if (val == newIndice) {
            coordNewIndice = key;
            foundVertexNewIndice = true;
        }
        if (foundVertexA && foundVertexB && foundVertexNewIndice) {
            break;
        }
    }
    if (!foundVertexA || !foundVertexB) {
        std::cerr << "Error: Could not find coordinates for vertex or neighbor to remove." << std::endl;
        return;
    }
}

void decimatePartTriangle(StreamMeshData& meshData, int vertexA, int vertexB, int newIndice,
                          int oldIndice, Vertex& coordVertexA, Vertex& coordVertexB,
                          Vertex& coordNewIndice) {

    // Get the triangles list of vertexA and vertexB
    std::vector<int>& triangleOfVertexB = meshData.triangleList[vertexB];
    std::vector<int>& triangleOfVertexA = meshData.triangleList[vertexA];
    // Initialize the new triangle list for the new vertex
    std::vector<int> trianglesToNewIndice = std::vector<int>();

    // Get the triangles map from the inCoreTriangleBuffer
    std::map<int, TriangleCoordinates>& inCoreTriangleBuffer = meshData.inCoreTriangleBuffer;

    // Get the size of the lists
    size_t sizeTriangleOfVertexB = triangleOfVertexB.size();
    size_t sizeTriangleOfVertexA = triangleOfVertexA.size();
    size_t i = 0, j = 0;
    // Loop through the triangles of vertexA and vertexB to update them and normally not delete the common triangles
    while (i < sizeTriangleOfVertexB && j < sizeTriangleOfVertexA) {
        // If a common triangle is found
        if (triangleOfVertexB[i] == triangleOfVertexA[j]) {
            std::cerr << "Error: This triangle must be already remove." << std::endl;
            return;
        }
        // if the triangle of vertexB is smaller than the triangle of vertexA
        else if (triangleOfVertexB[i] < triangleOfVertexA[j]) {
            TriangleCoordinates & triangle = inCoreTriangleBuffer[triangleOfVertexB[i]];
            // Find the old coordinate in the triangle to replace it with the new coordinate
            if (triangle.v1 == coordVertexB) {
                triangle.v1 = coordNewIndice;
            }
            else if (triangle.v2 == coordVertexB) {
                triangle.v2 = coordNewIndice;
            }
            else if (triangle.v3 == coordVertexB) {
                triangle.v3 = coordNewIndice;
            }
            // Add the triangle to the new triangle list
            trianglesToNewIndice.push_back(triangleOfVertexB[i]);
            ++i;
        }
        // if the triangle of vertexA is smaller than the triangle of vertexB
        else {
            TriangleCoordinates & triangle = inCoreTriangleBuffer[triangleOfVertexA[j]];
            // Find the old coordinate in the triangle to replace it with the new coordinate
            if (triangle.v1 == coordVertexA) {
                triangle.v1 = coordNewIndice;
            }
            else if (triangle.v2 == coordVertexA) {
                triangle.v2 = coordNewIndice;
            }
            else if (triangle.v3 == coordVertexA) {
                triangle.v3 = coordNewIndice;
            }
            // Add the triangle to the new triangle list
            trianglesToNewIndice.push_back(triangleOfVertexA[j]);
            ++j;
        }
    }
    // Loop through the remaining triangles of vertexB:
    while (i < sizeTriangleOfVertexB) {
        TriangleCoordinates & triangle = inCoreTriangleBuffer[triangleOfVertexB[i]];
        // Find the old coordinate in the triangle to replace it with the new coordinate
        if (triangle.v1 == coordVertexB) {
            triangle.v1 = coordNewIndice;
        }
        else if (triangle.v2 == coordVertexB) {
            triangle.v2 = coordNewIndice;
        }
        else if (triangle.v3 == coordVertexB) {
            triangle.v3 = coordNewIndice;
        }
        // Add the triangle to the new triangle list
        trianglesToNewIndice.push_back(triangleOfVertexB[i]);
        ++i;
    }

    while (j < sizeTriangleOfVertexA) {
        TriangleCoordinates & triangle = inCoreTriangleBuffer[triangleOfVertexA[j]];
        // Find the old coordinate in the triangle to replace it with the new coordinate
        if (triangle.v1 == coordVertexA) {
            triangle.v1 = coordNewIndice;
        }
        else if (triangle.v2 == coordVertexA) {
            triangle.v2 = coordNewIndice;
        }
        else if (triangle.v3 == coordVertexA) {
            triangle.v3 = coordNewIndice;
        }
        // Add the triangle to the new triangle list
        trianglesToNewIndice.push_back(triangleOfVertexA[j]);
        ++j;
    }

    // Remove the oldIndice from the triangle list and keep the newIndice
    meshData.triangleList.erase(oldIndice);
    meshData.triangleList[newIndice] = trianglesToNewIndice;
}


// /!\ WARNING /!\ WE NEED TO KEEP THE ADJACENCY LISTS SORTED IN ORDER TO KEEP THE ALGORITHM WORKING
bool decimateThisEdge(int vertexA, int vertexB, StreamMeshData& meshData) {
    // I made the assumption that we will always take the smallest index of the two
    // no matter if it's vertexA that we remove or vertexB,
    // plus it allows to keep the adjacency lists sorted.
    int newIndice, oldIndice;
    if (vertexA < vertexB) {
        newIndice = vertexA;
        oldIndice = vertexB;
    }
    else {
        newIndice = vertexB;
        oldIndice = vertexA;
    }

    // If the vertex is not in the border, it means that it has no neighbors in the adjacency list
    // (all the neighbors are in the adjacency list already seen)
    // So we simply remove the key of the one that no longer exists
    meshData.adjacencyList.erase(oldIndice);

    // Part Decimate AdjacencyList : to update the adjacency list of the new vertex
    decimatePartAdjacencyListAlreadySeen(meshData, vertexA, vertexB, newIndice, oldIndice);

    // Part Decimate TriangleQuadricMap : to update the quadric of the new vertex
    meshData.triangleQuadricMap[newIndice] = addQuadric(meshData.triangleQuadricMap[vertexA],
                                                        meshData.triangleQuadricMap[vertexB]);
    meshData.triangleQuadricMap.erase(oldIndice);

    // Get the coordinates of the vertexA, vertexB and TEMPORARY newIndice
    Vertex coordVertexA, coordVertexB, coordNewIndice;
    findCoordInVertexMap(meshData, vertexA, vertexB, newIndice, coordVertexA,
                         coordVertexB, coordNewIndice);

    // Part Decimate TriangleList : to update the triangle list of the new vertex and the triangles coordinates
    decimatePartTriangle(meshData, vertexA, vertexB, newIndice, oldIndice, coordVertexA,
                         coordVertexB, coordNewIndice);

    // Part Decimate VertexMap : to update the vertex map of the new vertex
    // In the vertex map, we will remove both of the old vertices and add the new vertex
    meshData.vertexMap.erase(coordVertexA);
    meshData.vertexMap.erase(coordVertexB);
    meshData.vertexMap[coordNewIndice] = newIndice;

    // Part Decimate VertexNotInBorder : to update the vertexNotInBorder
    // In the vertexNotInBorder, we will remove the old vertex and keep the new vertex
    for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
        if (meshData.vertexNotInBorder[i] == oldIndice) {
            meshData.vertexNotInBorder.erase(meshData.vertexNotInBorder.begin() + i);
            break;
        }
    }

    return true;
}

// Sert comme premier test, mais a modifié ainsi que les paramètres d'entrée et autre
bool decimate(float decimationPercentage, StreamMeshData& meshData) {
    int indiceRandomInVertexNotInBorder = rand() % meshData.vertexNotInBorder.size();
    int vertexToRemove = meshData.vertexNotInBorder[indiceRandomInVertexNotInBorder];
    //On choisit parmi ces voisins un voisin aléatoire qui est dans vertexNotInBorder en procédant aléatoirement par élimination
    std::vector<int> neighbors = meshData.adjacencyListAlreadySeen[vertexToRemove];
    int indiceRandomInNeighbors = rand() % neighbors.size();
    int neighborToRemove = neighbors[indiceRandomInNeighbors];
    while (neighbors.size() > 0 && std::find(meshData.vertexNotInBorder.begin(), meshData.vertexNotInBorder.end(),
                                             neighborToRemove) == meshData.vertexNotInBorder.end()) {
        // On retire le voisin de la liste
        neighbors.erase(neighbors.begin() + indiceRandomInNeighbors);
        // On choisit un voisin aléatoire parmi les voisins restants
        indiceRandomInNeighbors = rand() % neighbors.size();
        neighborToRemove = neighbors[indiceRandomInNeighbors];
    }
    if (neighbors.size() == 0) {
        std::cerr << "Error: No valid neighbor found to remove." << std::endl;
        return false;
    }

    std::cout << "Vertex to remove: " << vertexToRemove << std::endl;
    std::cout << "Neighbor to remove: " << neighborToRemove << std::endl;

    decimateThisEdge(vertexToRemove, neighborToRemove, meshData);
    return true;
}
