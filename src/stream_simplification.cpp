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


void writeHeader(std::ofstream &outputFile, int faceCount, bool isBinary = true) {
    if (!outputFile.is_open()) {
        std::cerr << "Error: Output file not open for writing header." << std::endl;
        return;
    }
    
    if (isBinary) {
        outputFile << "format: binary_little_endian" << std::endl;
    } else {
        outputFile << "format: ascii" << std::endl; 
    }
    
    outputFile << "face_count: " << faceCount << std::endl;
    outputFile << "END_HEADER" << std::endl;
}

int getTriangleCount(const std::string &filename) {
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

void displayFromInputFile(const std::string &inputFilePath, int startIndex, int endIndex)
{
    std::vector<std::array<float, 3>> unReadVertices;
    std::vector<std::array<int, 3>> unReadFaces;

    std::cout << "Extract vertices from : " << inputFilePath << std::endl;

    std::vector<TriangleCoordinates> unReadTriangles = extractTrianglesFromBinary(inputFilePath, startIndex, endIndex);

    std::cout << "Triangles extracted FROM INPUT : " << unReadTriangles.size() << std::endl;
    if ( unReadTriangles.size() != 0)
    {

        for (int i = 0; i < unReadTriangles.size(); i++)
        {
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

void displayFromOutputFile(const std::string &outputFilePath, int startIndex, int endIndex)
{
    std::vector<std::array<float, 3>> writtenVertices;
    std::vector<std::array<int, 3>> writtenFaces;

    std::cout << "Extract vertices from : " << outputFilePath << std::endl;

    std::vector<TriangleCoordinates> writtenTriangles = extractTrianglesFromBinary(outputFilePath, startIndex, endIndex);

    std::cout << "Triangles extracted FROM OUTPUT: " << writtenTriangles.size() << std::endl;
    if ( writtenTriangles.size() != 0)
    {

        for (int i = 0; i < writtenTriangles.size(); i++)
        {
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

void displayFromBuffer(std::vector<TriangleCoordinates> &inCoreTriangleBuffer, int nbTrianglesInCore)
{
    std::vector<std::array<float, 3>> inCoreVertices;
    std::vector<std::array<int, 3>> inCoreFaces;

    for (int i = 0; i < nbTrianglesInCore; i++)
    {
        inCoreVertices.push_back({inCoreTriangleBuffer[i].v1.x, inCoreTriangleBuffer[i].v1.y, inCoreTriangleBuffer[i].v1.z});
        inCoreVertices.push_back({inCoreTriangleBuffer[i].v2.x, inCoreTriangleBuffer[i].v2.y, inCoreTriangleBuffer[i].v2.z});
        inCoreVertices.push_back({inCoreTriangleBuffer[i].v3.x, inCoreTriangleBuffer[i].v3.y, inCoreTriangleBuffer[i].v3.z});
        inCoreFaces.push_back({3 * i, 3 * i + 1, 3 * i + 2});
    }

    polyscope::registerSurfaceMesh("In core mesh", inCoreVertices, inCoreFaces);
    auto meshPtr2 = polyscope::getSurfaceMesh("In core mesh");
    meshPtr2->setSurfaceColor(glm::vec3(0.8, 0.2, 0.2));
}

bool streamSimplificationVisualization(const std::string &inputFileName, const std::string &outputFileName, int maxTrianglesInBuffer, float decimationPercentage, bool visualizeSimplification)
{
    std::ifstream inputFile(inputFileName, std::ios::binary);
    if (!inputFile.is_open())
    {
        std::cerr << "Error: Could not open file " << inputFileName << ".\n";
        return false;
    }
    else
    {
        std::cout << "Input file " << inputFileName << " oppened" << ".\n";
    }

    std::ofstream outputFile(outputFileName, std::ios::binary | std::ios::app);
    if (!outputFile.is_open())
    {
        std::cerr << "Error: Could not open file " << outputFileName << ".\n";
        return false;
    }
    else
    {
        std::cout << "Output file " << outputFileName << " oppened" << ".\n";
    }

    int nbTrianglesRead = 0;
    int nbTrianglesInCore = 0;
    int nbTrianglesWritten = 0;
    int nbTriangleTotal = getTriangleCount(inputFileName); 

    writeHeader(outputFile, nbTriangleTotal, true);

    std::vector<TriangleCoordinates> inCoreTriangleBuffer(maxTrianglesInBuffer);

    // modif la facon dont les triangles sont trié dans le bianire en prennant le barycentre
    initBuffer(inputFile, inCoreTriangleBuffer, maxTrianglesInBuffer*0.5, decimationPercentage, &nbTrianglesInCore);
    // float R = trianglesInCore / trianglesRead - (trianglesWritten/decimationPercentage);

    std::cout << "======INIT======" << std::endl;
    std::cout << "Read : " << nbTrianglesRead << std::endl;
    std::cout << "In Core : " << nbTrianglesInCore << std::endl;
    std::cout << "======START LOOP======" << std::endl;

    while (!inputFile.eof())
    {
        read(inputFile, inCoreTriangleBuffer, maxTrianglesInBuffer*0.5, &nbTrianglesInCore);
        std::cout << "In Core : " << nbTrianglesInCore << std::endl;
        std::cout << "Written  : " << nbTrianglesWritten << std::endl;


        // decimate ((1-decimationPercentage) * maxTrianglesInBuffer / 4)

        if(visualizeSimplification){
            displayFromOutputFile(outputFileName, 0, nbTrianglesWritten);
            displayFromBuffer(inCoreTriangleBuffer, nbTrianglesInCore);
            if ( nbTrianglesWritten + nbTrianglesInCore < nbTriangleTotal)
            {
                displayFromInputFile(inputFileName, nbTrianglesWritten + nbTrianglesInCore, nbTriangleTotal);
            }
            polyscope::show();
        }

        // normalement write(outputFile, inCoreTriangleBuffer, maxTrianglesInBuffer * 0.5, &nbTrianglesWritten, &nbTrianglesInCore);
        write(outputFile, inCoreTriangleBuffer, nbTrianglesInCore, &nbTrianglesWritten, &nbTrianglesInCore);
    }

    // ici quand on vide le buffer, il faut faire un truc (voir papier)
    if(inCoreTriangleBuffer.size() > 0){
        write(outputFile, inCoreTriangleBuffer, nbTrianglesInCore, &nbTrianglesWritten, &nbTrianglesInCore);
    } 

    if(visualizeSimplification){
        displayFromOutputFile(outputFileName, 0, nbTrianglesWritten);
        polyscope::removeSurfaceMesh("Unread Mesh");
        polyscope::removeSurfaceMesh("In core mesh");
        polyscope::show();
    }


    return true;
}

// alterne reading / decimations until R == p
bool initBuffer(std::ifstream &inputFile, std::vector<TriangleCoordinates> &inCoreTriangleBuffer, int numberToRead, float decimationPercentage, int *nbTrianglesInCore)
{

    // skip le header
    std::string line;
    while (std::getline(inputFile, line))
    {
        if (line == "END_HEADER")
            break;
    }

    int n = 1.0f / decimationPercentage;
    std::cout << " n : " << n << std::endl;
    inputFile.read(reinterpret_cast<char *>(inCoreTriangleBuffer.data()), numberToRead * sizeof(TriangleCoordinates));
    *nbTrianglesInCore += inputFile.gcount() / sizeof(TriangleCoordinates);

    // for (int i=0; i < n; i++) {
    // inputFile.read(reinterpret_cast<char*>(inCoreTriangleBuffer.data()), (maxTrianglesInBuffer/2) * sizeof(TriangleCoordinates));
    // decimate(inCoreTriangleBuffer, maxTrianglesInBuffer/4);
    //}

    // apres init buffer on devrait avoir lu au total n/2 n/2 Nmax triangles et ecrit 0
    return true;
}


bool read(std::ifstream &inputFile, std::vector<TriangleCoordinates> &triangleBuffer, int numberToRead, int *trianglesInCore)
{
    inputFile.read(reinterpret_cast<char *>(triangleBuffer.data() + *trianglesInCore), numberToRead * sizeof(TriangleCoordinates));
    *trianglesInCore += inputFile.gcount() / sizeof(TriangleCoordinates);

    return true;
}

bool write(std::ofstream &outputFile, std::vector<TriangleCoordinates> & inCoreTriangleBuffer, int numberToWrite, int *trianglesWritten, int *trianglesInCore)
{
    // ajouter le truc pour choisir aléatoirement puis prendre le trianqle avec la plus grande erreur ?
    std::cout << "Writting " << numberToWrite << " Faces to outputfile " << std::endl;

    // la j'ecrit tout ce que qu'il y a dans le buffer pour le test de visualisation
    outputFile.write(reinterpret_cast<char *>(inCoreTriangleBuffer.data()), numberToWrite * sizeof(TriangleCoordinates));
    
    inCoreTriangleBuffer.clear();
    *trianglesInCore = 0;
    *trianglesWritten += numberToWrite;

    return true;
}


bool decimate()
{
    return true;
}
