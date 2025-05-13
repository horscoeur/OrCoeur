#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unistd.h>
#include <glm/glm.hpp>

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

    //std::cout << "Extract vertices from : " << inputFilePath << std::endl;

    std::vector<TriangleCoordinates> unReadTriangles = extractTrianglesFromBinary(inputFilePath, startIndex, endIndex);

    //std::cout << "Triangles extracted FROM INPUT : " << unReadTriangles.size() << std::endl;
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

void displayFromOutputFile(const std::string& outputFilePath) {
    std::vector<std::array<float, 3>> writtenVertices;
    std::vector<std::array<int, 3>> writtenFaces;

    //std::cout << "Extract vertices from : " << outputFilePath << std::endl;

    std::vector<TriangleCoordinates> writtenTriangles = extractTrianglesFromBinary(outputFilePath);

    //std::cout << "Triangles extracted FROM OUTPUT: " << writtenTriangles.size() << std::endl;
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
    }else {
        std::cout << "No triangles extracted from output file." << std::endl;
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

void writeRemainingTriangles(std::ofstream& outputFile, StreamMeshData& meshData) {
    if (!outputFile.is_open()) {
        std::cerr << "Error: Output file not open for writing remaining triangles." << std::endl;
        return;
    }

    // Write the remaining triangles in the buffer to the output file
    for (const auto& triangle : meshData.inCoreTriangleBuffer) {
        outputFile.write(reinterpret_cast<const char*>(&triangle.second), sizeof(TriangleCoordinates));
        meshData.inCoreTriangleBuffer.erase(triangle.first);
    }
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

    std::remove(outputFileName.c_str());  // Supprime le fichier s'il existe
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

    initBuffer(inputFile, maxTrianglesInBuffer * 0.5, decimationPercentage, meshData);
    //float R = meshData.inCoreTriangleBuffer.size() / nbTrianglesRead - (nbTrianglesWritten/decimationPercentage);

    /*
    std::cout << "======INIT======" << std::endl;
    std::cout << "Read : " << nbTrianglesRead << std::endl;
    std::cout << "Written : "<< nbTrianglesRead << std::endl;
    std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;
    std::cout << "======START LOOP======" << std::endl;
    */

    while (!inputFile.eof()) {
        std::cout << "======START ITERATION======" << std::endl;
        std::cout << "Reading " << maxTrianglesInBuffer * 0.5 << " Faces from inputfile\n";
        read(inputFile, maxTrianglesInBuffer * 0.5, meshData);


        int nb_decimated = (1-decimationPercentage) * maxTrianglesInBuffer / 4;
        std::cout << "Nb to be decimated : " << nb_decimated << std::endl;
        decimate (nb_decimated, meshData);

        if (visualizeSimplification) {
            displayFromOutputFile(outputFileName);
            displayFromBuffer(meshData.inCoreTriangleBuffer);
            displayBorderTriangles(meshData);  // Ajoutez cette ligne
            if (meshData.inCoreTriangleBuffer.size() < nbTriangleTotal) {
                displayFromInputFile(inputFileName, meshData.inCoreTriangleBuffer.size(), nbTriangleTotal);
            }
            polyscope::show();
        }

        if (meshData.inCoreTriangleBuffer.size() == nbTriangleTotal) {
            std::cout << "All triangles read, exiting loop." << std::endl;
            break;
        }

        //std::cout << "====== Je suis à la fin de cette boucle ======" << std::endl;
        //std::cout << "L'etat de input file est de " << inputFile.eof() << std::endl;
        if (inputFile.eof()) {
            std::cout << "End of file reached." << std::endl;
            break;
        }
        // normalement write(outputFile, inCoreTriangleBuffer, maxTrianglesInBuffer * 0.5, &nbTrianglesWritten, &nbTrianglesInCore);
        std::cout << "Writing " << maxTrianglesInBuffer * 0.5 << " Faces to outputfile\n";
        write(outputFile, meshData, (decimationPercentage/2) * maxTrianglesInBuffer , &nbTrianglesWritten);
        std::cout << "======ITERATION INFO======"<< std::endl;
        std::cout << "Read : " << nbTrianglesRead << std::endl;
        std::cout << "Written : " << nbTrianglesWritten << std::endl;
        std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;
        std::cout << "======ITERATION INFO======"<< std::endl;
    }
    
    //decimate((1-decimationPercentage * (1/decimationPercentage)) * maxTrianglesInBuffer / 4, meshData);

    // ici quand on vide le buffer, il faut faire un truc (voir papier)
    if (meshData.inCoreTriangleBuffer.size() > 0) {
        int n = 1.0f / decimationPercentage;
       //write(outputFile, meshData, (n * decimationPercentage * maxTrianglesInBuffer) * 0.5, &nbTrianglesWritten);
       writeRemainingTriangles(outputFile, meshData);
    }

    if (visualizeSimplification) {
        displayFromOutputFile(outputFileName);
        polyscope::removeSurfaceMesh("Unread Mesh");
        polyscope::removeSurfaceMesh("In core mesh");
        polyscope::show();
    }
    std::cout << "======END======" << std::endl;
    std::cout << "Read : " << nbTrianglesRead << std::endl;
    std::cout << "Written : " << nbTrianglesWritten << std::endl;
    std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;


    return true;
}


bool initBuffer(std::ifstream& inputFile, int numberToRead, float decimationPercentage,
                StreamMeshData& meshData) {

    std::string line;
    while (std::getline(inputFile, line)) {
        if (line == "END_HEADER")
            break;
    }

    int n = 1.0f / decimationPercentage;
    read(inputFile, numberToRead, meshData);
    int numberToDecimate = numberToRead * 0.5;
    for (int i=0; i < n-1; i++) {
        read(inputFile, numberToRead, meshData);
        decimate(numberToDecimate, meshData);
    }

    std::cout << "======INFO INIT ENDED======" << std::endl;
    std::cout << "Read : " << meshData.unique_triangle_index << std::endl;
    std::cout << "In Core : " << meshData.inCoreTriangleBuffer.size() << std::endl;
    std::cout << "Written : 0 " << std::endl;
    std::cout << "n : " << n << std::endl;
    std::cout << "decimationPercentage : " << decimationPercentage << std::endl;
    std::cout << "numberToRead : " << numberToRead << std::endl;
    std::cout << "======INFO INIT ENDED======" << std::endl;
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
    //std::cout << "Reading " << numberToRead << " Faces from inputfile\n";

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
            meshData.vertexSimplified[meshData.unique_triangle_index] = false;
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

void removeZeroTriangles(StreamMeshData& meshData) {
    std::vector<int> trianglesToRemove;
    

    for (const auto& [triangleId, triangle] : meshData.inCoreTriangleBuffer) {
        bool hasZeroCoordinates = 
            (triangle.v1.x == 0 && triangle.v1.y == 0 && triangle.v1.z == 0) ||
            (triangle.v2.x == 0 && triangle.v2.y == 0 && triangle.v2.z == 0) ||
            (triangle.v3.x == 0 && triangle.v3.y == 0 && triangle.v3.z == 0);
            
        bool hasDuplicateVertices = 
            (triangle.v1.x == triangle.v2.x && triangle.v1.y == triangle.v2.y && triangle.v1.z == triangle.v2.z) ||
            (triangle.v1.x == triangle.v3.x && triangle.v1.y == triangle.v3.y && triangle.v1.z == triangle.v3.z) ||
            (triangle.v2.x == triangle.v3.x && triangle.v2.y == triangle.v3.y && triangle.v2.z == triangle.v3.z);
            
        if (hasZeroCoordinates || hasDuplicateVertices) {
            trianglesToRemove.push_back(triangleId);
        }
    }
    
    for (int triangleId : trianglesToRemove) {
        auto& triangle = meshData.inCoreTriangleBuffer[triangleId];
        
        std::vector<int> vertexIds;
        if (meshData.vertexMap.find(triangle.v1) != meshData.vertexMap.end())
            vertexIds.push_back(meshData.vertexMap[triangle.v1]);
        if (meshData.vertexMap.find(triangle.v2) != meshData.vertexMap.end())
            vertexIds.push_back(meshData.vertexMap[triangle.v2]);
        if (meshData.vertexMap.find(triangle.v3) != meshData.vertexMap.end())
            vertexIds.push_back(meshData.vertexMap[triangle.v3]);
        
        for (int vertexId : vertexIds) {
            if (meshData.triangleList.find(vertexId) != meshData.triangleList.end()) {
                auto& triangles = meshData.triangleList[vertexId];
                triangles.erase(std::remove(triangles.begin(), triangles.end(), triangleId), triangles.end());
            }
        }
        
        meshData.inCoreTriangleBuffer.erase(triangleId);
    }
    
    if (!trianglesToRemove.empty()) {
        std::cout << "Removed " << trianglesToRemove.size() << " invalid triangles with zero coordinates." << std::endl;
    }
}


std::vector<int> getInBorderTriangles(StreamMeshData& meshData) {
    removeZeroTriangles(meshData); 

    std::vector<int> trianglesInBorder;
    
    for (const auto& [triangleId, triangle] : meshData.inCoreTriangleBuffer) {
        int v1 = meshData.vertexMap[triangle.v1];
        int v2 = meshData.vertexMap[triangle.v2];
        int v3 = meshData.vertexMap[triangle.v3];
        
        bool v1InBorder = std::find(meshData.vertexNotInBorder.begin(), 
                                    meshData.vertexNotInBorder.end(), 
                                    v1) 
                                    == meshData.vertexNotInBorder.end() ;
        bool v2InBorder = std::find(meshData.vertexNotInBorder.begin(), 
                                    meshData.vertexNotInBorder.end(), 
                                    v2) 
                                    == meshData.vertexNotInBorder.end() ;
        bool v3InBorder = std::find(meshData.vertexNotInBorder.begin(), 
                                    meshData.vertexNotInBorder.end(), 
                                    v3) 
                                    == meshData.vertexNotInBorder.end();
                         
        if (v1InBorder || v2InBorder || v3InBorder) {
            trianglesInBorder.push_back(triangleId);
        }
    }
    
    return trianglesInBorder;
}

std::vector<int> getInBorderVertices(StreamMeshData& meshData) {
    removeZeroTriangles(meshData); 

    std::vector<int> verticesInBorder;
    
    for (const auto& [vertex, vertexId] : meshData.vertexMap) {

        bool inBorder = std::find(meshData.vertexNotInBorder.begin(), 
                                 meshData.vertexNotInBorder.end(), 
                                 vertexId) 
                                 == meshData.vertexNotInBorder.end();        
        if (inBorder) {
            verticesInBorder.push_back(vertexId);
        } 
    }
    
    return verticesInBorder;  
}

void displayBorderTriangles(StreamMeshData& meshData) {
    std::vector<int> borderTriangles = getInBorderTriangles(meshData);
    
    if (borderTriangles.empty()) {
        std::cout << "No border to be displayed" << std::endl;
        return;
    }else {
        std::cout << "Triangles in border displayed : " << borderTriangles.size() << std::endl;
    }
    
    std::vector<std::array<float, 3>> borderVertices;
    std::vector<std::array<int, 3>> borderFaces;
    
    int faceIndex = 0;
    for (const int& triangleId : borderTriangles) {
        const auto& triangle = meshData.inCoreTriangleBuffer[triangleId];
        
        borderVertices.push_back({triangle.v1.x, triangle.v1.y, triangle.v1.z});
        borderVertices.push_back({triangle.v2.x, triangle.v2.y, triangle.v2.z});
        borderVertices.push_back({triangle.v3.x, triangle.v3.y, triangle.v3.z});
        
        borderFaces.push_back({3 * faceIndex, 3 * faceIndex + 1, 3 * faceIndex + 2});
        faceIndex++;
    }
    
    // Enregistrer et afficher le maillage
    polyscope::registerSurfaceMesh("Border Triangles", borderVertices, borderFaces);
    auto meshPtr = polyscope::getSurfaceMesh("Border Triangles");
    // Couleur distincte pour les triangles en bordure (jaune)
    meshPtr->setSurfaceColor(glm::vec3(0.9, 0.9, 0.0));
}


std::vector<int> getToBeWrittenTriangles(StreamMeshData& meshData, int numberToWrite) { // TODO : quand on va simplifier ne plus peut etre prioritiser en fonction de l'axe de la simplification
    std::vector<int> trianglesInBorder = getInBorderTriangles(meshData);
    std::cout << "Triangles in border : " << trianglesInBorder.size() << std::endl;
    
    // Créer un set pour une recherche efficace
    std::unordered_set<int> borderSet(trianglesInBorder.begin(), trianglesInBorder.end());
    
    // Créer un vecteur pour les triangles non-bordure
    std::vector<int> trianglesInBuffer;
    trianglesInBuffer.reserve(meshData.inCoreTriangleBuffer.size());
    
    // Ajouter tous les triangles qui ne sont pas dans borderSet
    for (const auto& [triangleId, _] : meshData.inCoreTriangleBuffer) {
        if (borderSet.find(triangleId) == borderSet.end()) {
            trianglesInBuffer.push_back(triangleId);
        }
    }
    
    std::cout << "Non-border triangles : " << trianglesInBuffer.size() << std::endl;
    
    // ca pas certain d'en avoir besoin
    if (trianglesInBuffer.empty()) {
        std::cout << "No triangles in border" << std::endl;
        //TODO prendre un triangle au hasard dans le buffer
    }else if (trianglesInBuffer.size() < numberToWrite) {
        std::cout << "Not enough triangles in border" << std::endl;
        //TODO ajouter des triangles au hasard dans le buffer ?
    }

    return trianglesInBuffer;
} 




// DEPRECATED FOR NOW
bool write(std::ofstream &outputFile, StreamMeshData &meshData, int numberToWrite, int *trianglesWritten)
{
    int numberWritten = 0;
    const int NB_CANDIDATES = 3;

    std::vector<int> trianglesToBeWritten = getToBeWrittenTriangles(meshData, numberToWrite);
    std::cout << "Valid Triangles : " << trianglesToBeWritten.size() << std::endl;
    std::cout << "Number to write : " << numberToWrite << std::endl;

    //assert(trianglesToBeWritten.size() > numberToWrite);

    for (size_t i = 0; i < numberToWrite; i++)
    {

        float error = -1e10;
        int triangleIndexToBeDeleted = -1;

        // nb_candidates * l'opération de selection
        for (size_t j = 0; j < NB_CANDIDATES; j++)
        {

            if(trianglesToBeWritten.size() == 0)
            {
                std::cout << "No triangles left to be written" << std::endl;
                break;
            }

            int randomTriangleIndex = rand() % trianglesToBeWritten.size();
            int triangleIndex = trianglesToBeWritten[randomTriangleIndex];

            if (meshData.inCoreTriangleBuffer.find(triangleIndex) == meshData.inCoreTriangleBuffer.end()) {
                trianglesToBeWritten.erase(trianglesToBeWritten.begin() + randomTriangleIndex);
                continue;
            }

            TriangleCoordinates triangle = meshData.inCoreTriangleBuffer[triangleIndex];

            Vertex triangleVertexA = triangle.v1;
            Vertex triangleVertexB = triangle.v2;
            Vertex triangleVertexC = triangle.v3;

            Vertex triangleCenter = {
                (triangleVertexA.x + triangleVertexB.x + triangleVertexC.x) / 3,
                (triangleVertexA.y + triangleVertexB.y + triangleVertexC.y) / 3,
                (triangleVertexA.z + triangleVertexB.z + triangleVertexC.z) / 3};

            int triangleIdV1 = meshData.vertexMap[triangleVertexA];
            int triangleIdV2 = meshData.vertexMap[triangleVertexB];
            int triangleIdV3 = meshData.vertexMap[triangleVertexC];

            Quadric triangleQuadric = meshData.triangleQuadricMap[triangleIdV1];
            triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[triangleIdV2]);
            triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[triangleIdV3]);

            float currentError = evaluateError(triangleQuadric, triangleCenter);

            if (currentError > error)
            {
                error = currentError;
                triangleIndexToBeDeleted = triangleIndex;
            }
        }

        if (triangleIndexToBeDeleted != -1)
        {
            auto &triangle = meshData.inCoreTriangleBuffer[triangleIndexToBeDeleted];

            // Vérifier si le triangle a des valeurs nulles ou incohérentes
            bool hasZeroCoordinates =
                (triangle.v1.x == 0 && triangle.v1.y == 0 && triangle.v1.z == 0) ||
                (triangle.v2.x == 0 && triangle.v2.y == 0 && triangle.v2.z == 0) ||
                (triangle.v3.x == 0 && triangle.v3.y == 0 && triangle.v3.z == 0);

            // Vérifier si deux sommets sont identiques (triangle dégénéré)
            bool hasDuplicateVertices =
                (triangle.v1.x == triangle.v2.x && triangle.v1.y == triangle.v2.y && triangle.v1.z == triangle.v2.z) ||
                (triangle.v1.x == triangle.v3.x && triangle.v1.y == triangle.v3.y && triangle.v1.z == triangle.v3.z) ||
                (triangle.v2.x == triangle.v3.x && triangle.v2.y == triangle.v3.y && triangle.v2.z == triangle.v3.z);

            // Vérifier si le triangle est plat (les trois points sont alignés)
            glm::vec3 v1(triangle.v2.x - triangle.v1.x, triangle.v2.y - triangle.v1.y, triangle.v2.z - triangle.v1.z);
            glm::vec3 v2(triangle.v3.x - triangle.v1.x, triangle.v3.y - triangle.v1.y, triangle.v3.z - triangle.v1.z);
            glm::vec3 normal = glm::cross(v1, v2);
            float area = 0.5f * glm::length(normal);
            bool isDegenerate = area < 1e-6f; // Si l'aire est proche de zéro

            // Vérifier les valeurs NaN ou infinies
            bool hasInvalidValues =
                std::isnan(triangle.v1.x) || std::isinf(triangle.v1.x) ||
                std::isnan(triangle.v1.y) || std::isinf(triangle.v1.y) ||
                std::isnan(triangle.v1.z) || std::isinf(triangle.v1.z) ||
                std::isnan(triangle.v2.x) || std::isinf(triangle.v2.x) ||
                std::isnan(triangle.v2.y) || std::isinf(triangle.v2.y) ||
                std::isnan(triangle.v2.z) || std::isinf(triangle.v2.z) ||
                std::isnan(triangle.v3.x) || std::isinf(triangle.v3.x) ||
                std::isnan(triangle.v3.y) || std::isinf(triangle.v3.y) ||
                std::isnan(triangle.v3.z) || std::isinf(triangle.v3.z);

            if (!hasZeroCoordinates && !hasDuplicateVertices && !isDegenerate && !hasInvalidValues)
            {
                if (outputFile.write(reinterpret_cast<char *>(&triangle), sizeof(TriangleCoordinates)))
                {
                    numberWritten++;

                    int v1Id = meshData.vertexMap[triangle.v1];
                    int v2Id = meshData.vertexMap[triangle.v2];
                    int v3Id = meshData.vertexMap[triangle.v3];

                    for (int vertexId : {v1Id, v2Id, v3Id}) {
                        auto& triangles = meshData.triangleList[vertexId];
                        triangles.erase(std::remove(triangles.begin(), triangles.end(), triangleIndexToBeDeleted), triangles.end());

                        if (triangles.empty()) {

                            for (auto it = meshData.vertexMap.begin(); it != meshData.vertexMap.end();) {
                                if (it->second == vertexId) {
                                    it = meshData.vertexMap.erase(it);
                                } else {
                                    ++it;
                                }
                            }

                            auto it = std::find(meshData.vertexNotInBorder.begin(), meshData.vertexNotInBorder.end(), vertexId);
                            if (it != meshData.vertexNotInBorder.end()) {
                                meshData.vertexNotInBorder.erase(it);
                            }

                            meshData.triangleList.erase(vertexId);
                            meshData.adjacencyList.erase(vertexId);
                            meshData.adjacencyListAlreadySeen.erase(vertexId);
                            meshData.triangleQuadricMap.erase(vertexId);
                            meshData.vertexSimplified.erase(vertexId);
                        }
                    }
                    meshData.inCoreTriangleBuffer.erase(triangleIndexToBeDeleted);
                }else {
                    std::cerr << "Error writing triangle to output file." << std::endl;
                }
            }
            else
            {
                /*
                std::cerr << "Triangle has invalid values or is degenerate." << std::endl;
                std::cout << "Triangle values: "
                          << triangle.v1.x << "," << triangle.v1.y << "," << triangle.v1.z << " | "
                          << triangle.v2.x << "," << triangle.v2.y << "," << triangle.v2.z << " | "
                          << triangle.v3.x << "," << triangle.v3.y << "," << triangle.v3.z << std::endl;
                */
            }
        }
    }

    *trianglesWritten += numberWritten;

    std::cout << "Number of triangles written : " << numberWritten << std::endl;
    return true;
}

void removeCommonTriangles(int vertexA, int vertexB, StreamMeshData& meshData) {
    // List of common triangles
    std::vector<int> commonTriangles;
    
    std::vector<int>& trianglesVertexA = meshData.triangleList[vertexA];
    std::vector<int>& trianglesVertexB = meshData.triangleList[vertexB];
    
    // Loops through the triangles (they need to be sorted)
    size_t i = 0, j = 0;
    while (i < trianglesVertexA.size() && j < trianglesVertexB.size()) {
        // if they have the same triangle push it to commonTriangles
        if (trianglesVertexA[i] == trianglesVertexB[j]) {
            commonTriangles.push_back(trianglesVertexA[i]);
            i++; j++;
        } 
        else if (trianglesVertexA[i] < trianglesVertexB[j]) {
            i++;
        } 
        else {
            j++;
        }
    }
    
    // Removes the common triangles 
    for (int triangleId : commonTriangles) {
        for (int v : {vertexA, vertexB}) { // surprime les triangles de vertexA et vertexB. TODO : le faire aussi dans le 3eme sommet
            auto &list = meshData.triangleList[v];
            list.erase(std::remove(list.begin(), list.end(), triangleId), list.end());
        }
        meshData.inCoreTriangleBuffer.erase(triangleId);
    }
}


bool isCollapseValid(int vertexA, int vertexB, Vertex &positionAfterCollapse, StreamMeshData &meshData)
{
    // check connectivity
    std::vector<int> &trianglesIdVertexA = meshData.triangleList[vertexA];
    std::vector<int> &trianglesIdVertexB = meshData.triangleList[vertexB];

    std::vector<int> commonTriangles;

    std::set_intersection(trianglesIdVertexA.begin(), trianglesIdVertexA.end(),
                          trianglesIdVertexB.begin(), trianglesIdVertexB.end(),
                          std::back_inserter(commonTriangles));

    if (commonTriangles.size() != 2)
    {
        return false;
    }

    std::vector<int> survivingTriangles;
    std::set_difference(trianglesIdVertexA.begin(), trianglesIdVertexA.end(),
                       commonTriangles.begin(), commonTriangles.end(),
                       std::back_inserter(survivingTriangles));
    std::set_difference(trianglesIdVertexB.begin(), trianglesIdVertexB.end(),
                       commonTriangles.begin(), commonTriangles.end(),
                       std::back_inserter(survivingTriangles));

    /*
    std::cout << "Checking collapse of edge (" << vertexA << "," << vertexB << ") to position ("
              << positionAfterCollapse.x << "," << positionAfterCollapse.y << "," 
              << positionAfterCollapse.z << ")\n";
    */

    // TODO : recuperer juste la position des vertices qui seront affectés par le collapse (ca récup tout)
    std::vector<Vertex> positions(meshData.actual_unique_id);
    for (auto &kv : meshData.vertexMap)
    {
        positions[kv.second] = kv.first;
    }

    std::vector<int> notCommonTriangles;
    std::set_symmetric_difference(trianglesIdVertexA.begin(), trianglesIdVertexA.end(),
                        trianglesIdVertexB.begin(), trianglesIdVertexB.end(),
                        std::back_inserter(notCommonTriangles));

    // lambda pour calculer la normale d'un triangle
    auto computeNormal = [&](int triangleId, const std::vector<Vertex> &pos)
    {
        const auto &tri = meshData.inCoreTriangleBuffer[triangleId];
        
        int idx0 = meshData.vertexMap[tri.v1];
        int idx1 = meshData.vertexMap[tri.v2];
        int idx2 = meshData.vertexMap[tri.v3];
        
        const Vertex &p0 = pos[idx0];
        const Vertex &p1 = pos[idx1];
        const Vertex &p2 = pos[idx2];
        
        /*
        std::cout << "Triangle " << triangleId << " vertices:\n"
                  << "  v0: (" << p0.x << "," << p0.y << "," << p0.z << ")\n"
                  << "  v1: (" << p1.x << "," << p1.y << "," << p1.z << ")\n"
                  << "  v2: (" << p2.x << "," << p2.y << "," << p2.z << ")\n";
        */
        glm::vec3 e1 = glm::vec3(p1.x - p0.x, p1.y - p0.y, p1.z - p0.z);
        glm::vec3 e2 = glm::vec3(p2.x - p0.x, p2.y - p0.y, p2.z - p0.z);
        
        glm::vec3 normal = glm::cross(e1, e2);
        normal = glm::normalize(normal);
        
        //std::cout << "  normal: (" << normal.x << "," << normal.y << "," << normal.z << ")\n";
        return normal;
    };

    std::unordered_map<int, glm::vec3> originalNormals;
    for (int t : notCommonTriangles){
        originalNormals[t] = computeNormal(t, positions);
        //std::cout << "Normal of triangle " << t << " : " << originalNormals[t].x << " " << originalNormals[t].y << " " << originalNormals[t].z << std::endl;
    }

    auto newPos = positions;
    newPos[vertexA] = positionAfterCollapse;
    newPos[vertexB] = positionAfterCollapse;

    for (int t : survivingTriangles)
    {
        glm::vec3 n0 = originalNormals[t];
        glm::vec3 n1 = computeNormal(t, newPos);
        float dot = glm::dot(n0, n1);
        //std::cout << "Triangle " << t << " dot product: " << dot << "\n";
        if (dot < 0){
            //std::cout << "Collapse invalid due to triangle " << t << " flipping.\n";
            return false;
        } 
    }

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

    removeCommonTriangles(vertexA, vertexB, meshData);

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

int getRandomNeighborNotInBorder(int vertex, StreamMeshData& meshData) {
    std::vector<int> verticesInBorder = getInBorderVertices(meshData);

    std::vector<int> & neighbors = meshData.adjacencyListAlreadySeen[vertex];
    if (neighbors.empty()) {
        return -1; 
    }

    std::vector<int> neighborsNotInBorder;
    for (int neighbor : neighbors) {
        // 1. Vérifier que le neighbor n'est pas en bordure
        if (std::find(verticesInBorder.begin(),
                      verticesInBorder.end(),
                      neighbor) == verticesInBorder.end()) {
            
            // 2. Vérifier que le neighbor existe dans vertexMap avec des coordonnées valides
            bool foundInVertexMap = false;
            Vertex neighborCoords;
            
            for (const auto& [vertexCoords, vertexId] : meshData.vertexMap) {
                if (vertexId == neighbor) {
                    foundInVertexMap = true;
                    neighborCoords = vertexCoords;
                    break;
                }
            }
            
            // 3. Vérifier que les coordonnées ne sont pas nulles
            bool isZeroCoordinate = (neighborCoords.x == 0 && 
                                    neighborCoords.y == 0 && 
                                    neighborCoords.z == 0);
            
            if (foundInVertexMap && !isZeroCoordinate) {
                neighborsNotInBorder.push_back(neighbor);
            }
        }
    }
    
    if (neighborsNotInBorder.empty()) {
        return -1; 
    }

    int randomIndex = rand() % neighborsNotInBorder.size();
    return neighborsNotInBorder[randomIndex];
}

// Sert comme premier test, mais a modifié ainsi que les paramètres d'entrée et autre
bool decimate(int nbToDecimate, StreamMeshData &meshData)
{
    if (meshData.vertexNotInBorder.size() == 0)
    {
        std::cout << "No vertex not in border, nothing to decimate." << std::endl;
        return true;
    }
    if (nbToDecimate == 0)
    {
        std::cout << "No vertex to decimate." << std::endl;
        return true;
    }
    int decimated = 0;

    const int NUM_CANDIDATES = 15;

    for (size_t i = 0; i < nbToDecimate - 1; i++)
    {
        float bestError = std::numeric_limits<float>::max();
        int bestVertexA = -1;
        int bestVertexB = -1;

        for (size_t j = 0; j < NUM_CANDIDATES; j++)
        {
            int indiceRandomInVertexNotInBorder = rand() % meshData.vertexNotInBorder.size();
            int vertexA = meshData.vertexNotInBorder[indiceRandomInVertexNotInBorder];
            int vertexB = getRandomNeighborNotInBorder(vertexA, meshData);
            if (vertexB == -1) {
                //std::cout << "No neighbor not in border for vertex " << vertexA << std::endl;
                continue;
            }

            int indexNewVertex;
            Vertex coordVertexA, coordVertexB, coordNewVertex;
            findCoordInVertexMap(meshData, vertexA, vertexB, indexNewVertex, coordVertexA, coordVertexB, coordNewVertex);

            if (vertexA < vertexB){
                coordNewVertex = coordVertexA;
            }else {
                coordNewVertex = coordVertexB;
            }

            //coordNewVertex = findOptimalVertex(combinedQuadric, coordVertexA, coordVertexB);
            //std::cout << "New vertex position: (" << coordNewVertex.x << "," << coordNewVertex.y << "," << coordNewVertex.z << ")\n";

            if(!isCollapseValid(vertexA, vertexB, coordNewVertex, meshData)){
                /*
                std::cout << "Collapse invalid for edge (" << vertexA << "," << vertexB << ") to position ("
                          << coordNewVertex.x << "," << coordNewVertex.y << "," 
                          << coordNewVertex.z << ")\n";
                */
                continue;
            };



            Quadric combinedQuadric = addQuadric(meshData.triangleQuadricMap[vertexA], meshData.triangleQuadricMap[vertexB]);

            float error = evaluateError(combinedQuadric, coordNewVertex);

            if (error <= bestError)
            {
                bestError = error;
                bestVertexA = vertexA;
                bestVertexB = vertexB;
            }
        }

        if (bestVertexA != -1 && bestVertexB != -1)
        {
            decimateThisEdge(bestVertexA, bestVertexB, meshData);
            meshData.vertexSimplified[bestVertexA] = true;
            meshData.vertexSimplified[bestVertexB] = true;
            //std::cout << "Decimated edge (" << bestVertexA << "," << bestVertexB << ") with error " << bestError << std::endl;
            decimated++;
        }
    }
    

    std::cout << "Decimated " << decimated << " edges." << std::endl;
    return true;
}
