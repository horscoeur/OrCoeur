#include "mesh_simplification.h"
#include "dereference_passes.h"
#include "merge_sort.h"
#include "dereference_passes.h"
#include "structures.h"

#define BUFFER_SIZE 1024


bool computeGridCellRepresentatives(const std::string &inputFilenamePlaneEquation, const std::string &outputFilename) {
    std::ifstream filePlaneEquation(inputFilenamePlaneEquation, std::ios::binary);
    if (!filePlaneEquation.is_open()) {
        std::cerr << "Error: Could not open file " << inputFilenamePlaneEquation << ".\n";
        return false;
    }

    std::ofstream outputFile(outputFilename);
    if (!outputFile.is_open()) {
        std::cerr << "Error: Could not open output files.\n";
        outputFile.close();
        return false;
    }

    GridPlaneEntry gridPlaneEntry;
    int currentGridIndex = -1;
    Quadric currentQuadric;

    // Read the plane equations file plane by plane and accumulate the quadrics for each grid cell
    while (filePlaneEquation.read(reinterpret_cast<char *>(&gridPlaneEntry), sizeof(GridPlaneEntry))) {
        if (currentGridIndex != gridPlaneEntry.gridIndex) {

            if (currentGridIndex != -1) {
                // Solve the quadric to find the optimal vertex position
                Vertex optimalVertex = findOptimalVertex(currentQuadric);
                
                // Write the grid index and optimal vertex to the output file
                outputFile.write(reinterpret_cast<const char*>(&currentGridIndex), sizeof(int));
                outputFile.write(reinterpret_cast<const char*>(&optimalVertex), sizeof(Vertex));
            }
            
            // Start a new quadric for the new grid cell
            currentGridIndex = gridPlaneEntry.gridIndex;
            currentQuadric = Quadric(); // Reset to a new quadric
        }

        currentQuadric = addQuadric(currentQuadric, computeQuadricPlaneEquation(gridPlaneEntry.planeEquation));
    }
     
    if (currentGridIndex != -1) {
        Vertex optimalVertex = findOptimalVertex(currentQuadric);
        outputFile.write(reinterpret_cast<const char*>(&currentGridIndex), sizeof(int));
        outputFile.write(reinterpret_cast<const char*>(&optimalVertex), sizeof(Vertex));
    }

    return true; 
}

void externalMergeSortGridPlaneEntry(const std::string &inputFile, const std::string &outputFile) {
    externalMergeSort<GridPlaneEntry>(inputFile, outputFile, [](const GridPlaneEntry &a, const GridPlaneEntry &b) {
        return a.gridIndex < b.gridIndex;
    });
}


bool dereferenceClusterPass1(const std::string &representativesFilename, const std::string &clusterFilename, const std::string &outputFilename) {
    // First, sort the triangle indices by v1
    std::string sortedTrianglesFile = "sorted_by_v1.bin";
    externalMergeSortTrianglesIndices(clusterFilename, sortedTrianglesFile);
    
    
    // Open files in binary mode
    std::ifstream sortedTriangles(sortedTrianglesFile, std::ios::binary);
    std::ifstream sortedReps(representativesFilename, std::ios::binary);
    std::ofstream output(outputFilename, std::ios::binary);
    
    if (!sortedTriangles.is_open() || !sortedReps.is_open() || !output.is_open()) {
        std::cerr << "Error: Unable to open files in deferenceClusterPasses1" << std::endl;
        std::remove(sortedTrianglesFile.c_str());
        return false;
    }
    
    // Buffers for reading triangle indices and writing output records
    std::vector<TriangleIndices> triangleBuffer(BUFFER_SIZE);
    std::vector<Triangle_Pass1> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);
    

    int currentGridIndex = -1;
    Vertex currentVertex;

    if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
        !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first representative." << std::endl;
        return false;
    }
    
    // Process the sorted triangle indices in blocks
    while (true) {
        sortedTriangles.read(reinterpret_cast<char*>(triangleBuffer.data()), BUFFER_SIZE * sizeof(TriangleIndices));
        size_t trianglesRead = sortedTriangles.gcount() / sizeof(TriangleIndices);
        if (trianglesRead == 0) 
            break;
        
        for (size_t i = 0; i < trianglesRead; i++) {
            const auto &tri = triangleBuffer[i];
            
            // Advance through the vertex file until we reach the vertex at index tri.v1
            while (currentGridIndex < tri.v1) {
                if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
                    !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                    
                    if (sortedReps.eof()) {
                        std::cerr << "Warning: Grid index " << tri.v1 << " not found in representatives" << std::endl;
                        currentGridIndex = -1; // End of file
                        break;
                    } else {
                        std::cerr << "Error reading representatives file" << std::endl;
                        return false;
                    }
                }
            }
            
            // Si on a trouvé l'indice correspondant
            if (currentGridIndex == tri.v1) {
                Triangle_Pass1 triPass1;
                triPass1.v1 = currentVertex;
                triPass1.v2 = tri.v2;
                triPass1.v3 = tri.v3;
                outputBuffer.push_back(triPass1);
            } else if (currentGridIndex > tri.v1) {
                // If we have passed the index, it means it does not exist
                std::cerr << "Warning: Grid index " << tri.v1 << " not found in representatives" << std::endl;
                
                // We need to reposition the file to read the next index
                sortedReps.seekg(-static_cast<int>(sizeof(int) + sizeof(Vertex)), std::ios::cur);
                if (!sortedReps.good()) {
                    std::cerr << "Error repositioning representatives file" << std::endl;
                    return false;
                }
            }
            
            if (outputBuffer.size() >= BUFFER_SIZE) {
                output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                           outputBuffer.size() * sizeof(Triangle_Pass1));
                outputBuffer.clear();
            }
        }
    }
    
    if (!outputBuffer.empty()) {
        output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                   outputBuffer.size() * sizeof(Triangle_Pass1));
    }
    
    sortedTriangles.close();
    sortedReps.close();
    output.close();
    std::remove(sortedTrianglesFile.c_str());
    
    return true;
}

bool dereferenceClusterPass2(const std::string &pass1Filename, const std::string &representativesFilename, const std::string &outputFilename) {
    std::string sortedPass1File = "sorted_by_v2.bin";
    externalMergeSortTrianglesPass1(pass1Filename, sortedPass1File);
    
    std::ifstream sortedPass1(sortedPass1File, std::ios::binary);
    std::ifstream sortedReps(representativesFilename, std::ios::binary);
    std::ofstream output(outputFilename, std::ios::binary);
    
    if (!sortedPass1.is_open() || !sortedReps.is_open() || !output.is_open()) {
        std::cerr << "Error: Unable to open files in deferenceClusterPasses2" << std::endl;
        std::remove(sortedPass1File.c_str());
        return false;
    }
    
    std::vector<Triangle_Pass1> triangleBuffer(BUFFER_SIZE);
    std::vector<Triangle_Pass2> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);
    
    int currentGridIndex = -1;
    Vertex currentVertex;
    if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
        !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first representative in pass 2." << std::endl;
        return false;
    }
    
    while (true) {
        sortedPass1.read(reinterpret_cast<char*>(triangleBuffer.data()), BUFFER_SIZE * sizeof(Triangle_Pass1));
        size_t trianglesRead = sortedPass1.gcount() / sizeof(Triangle_Pass1);
        if (trianglesRead == 0) break;
        
        for (size_t i = 0; i < trianglesRead; i++) {
            const auto &tri = triangleBuffer[i];
            
            if (tri.v2 < currentGridIndex) {
                sortedReps.clear();
                sortedReps.seekg(0);
                if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
                    !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                    std::cerr << "Error: Unable to reset representatives file in pass 2." << std::endl;
                    return false;
                }
            }
            
            while (currentGridIndex < tri.v2) {
                if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
                    !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                    
                    if (sortedReps.eof()) {
                        std::cerr << "Warning: Grid index " << tri.v2 << " not found in representatives" << std::endl;
                        currentGridIndex = -1;
                        break;
                    } else {
                        std::cerr << "Error reading representatives file in pass 2" << std::endl;
                        return false;
                    }
                }
            }
            
            if (currentGridIndex == tri.v2) {
                Triangle_Pass2 triPass2;
                triPass2.v1 = tri.v1;
                triPass2.v2 = currentVertex;
                triPass2.v3 = tri.v3;
                outputBuffer.push_back(triPass2);
            } else if (currentGridIndex > tri.v2) {
                std::cerr << "Warning: Grid index " << tri.v2 << " not found in representatives" << std::endl;
                
                sortedReps.seekg(-static_cast<int>(sizeof(int) + sizeof(Vertex)), std::ios::cur);
                if (!sortedReps.good()) {
                    std::cerr << "Error repositioning representatives file in pass 2" << std::endl;
                    return false;
                }
            }
            
            if (outputBuffer.size() >= BUFFER_SIZE) {
                output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                           outputBuffer.size() * sizeof(Triangle_Pass2));
                outputBuffer.clear();
            }
        }
    }
    
    if (!outputBuffer.empty()) {
        output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                   outputBuffer.size() * sizeof(Triangle_Pass2));
    }
    
    sortedPass1.close();
    sortedReps.close();
    output.close();
    std::remove(sortedPass1File.c_str());
    
    return true;
}

int dereferenceClusterPass3(const std::string &pass2Filename, const std::string &representativesFilename, const std::string &outputFilename) {
    std::string sortedPass2File = "sorted_by_v3.bin";
    externalMergeSortTrianglesPass2(pass2Filename, sortedPass2File);
    
    std::ifstream sortedPass2(sortedPass2File, std::ios::binary);
    std::ifstream sortedReps(representativesFilename, std::ios::binary);
    std::ofstream output(outputFilename, std::ios::binary);
    
    if (!sortedPass2.is_open() || !sortedReps.is_open() || !output.is_open()) {
        std::cerr << "Error: Unable to open files in deferenceClusterPasses3" << std::endl;
        std::remove(sortedPass2File.c_str());
        return -1;
    }
    
    std::vector<Triangle_Pass2> triangleBuffer(BUFFER_SIZE);
    std::vector<TriangleCoordinates> outputBuffer;
    outputBuffer.reserve(BUFFER_SIZE);
    
    int currentGridIndex = -1;
    Vertex currentVertex;
    if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
        !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
        std::cerr << "Error: Unable to read the first representative in pass 3." << std::endl;
        return -1;
    }
    
    int nbFaces = 0;
    
    while (true) {
        sortedPass2.read(reinterpret_cast<char*>(triangleBuffer.data()), BUFFER_SIZE * sizeof(Triangle_Pass2));
        size_t trianglesRead = sortedPass2.gcount() / sizeof(Triangle_Pass2);
        if (trianglesRead == 0) break;
        
        for (size_t i = 0; i < trianglesRead; i++) {
            const auto &tri = triangleBuffer[i];
            
            if (tri.v3 < currentGridIndex) {
                sortedReps.clear();
                sortedReps.seekg(0);
                if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
                    !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                    std::cerr << "Error: Unable to reset representatives file in pass 3." << std::endl;
                    return -1;
                }
            }
            
            while (currentGridIndex < tri.v3) {
                if (!sortedReps.read(reinterpret_cast<char*>(&currentGridIndex), sizeof(int)) ||
                    !sortedReps.read(reinterpret_cast<char*>(&currentVertex), sizeof(Vertex))) {
                    
                    if (sortedReps.eof()) {
                        std::cerr << "Warning: Grid index " << tri.v3 << " not found in representatives" << std::endl;
                        currentGridIndex = -1;
                        break;
                    } else {
                        std::cerr << "Error reading representatives file in pass 3" << std::endl;
                        return -1;
                    }
                }
            }
            
            if (currentGridIndex == tri.v3) {
                TriangleCoordinates finalTri;
                finalTri.v1 = tri.v1;
                finalTri.v2 = tri.v2;
                finalTri.v3 = currentVertex;
                outputBuffer.push_back(finalTri);
                nbFaces++;
            } else if (currentGridIndex > tri.v3) {
                std::cerr << "Warning: Grid index " << tri.v3 << " not found in representatives" << std::endl;
                
                sortedReps.seekg(-static_cast<int>(sizeof(int) + sizeof(Vertex)), std::ios::cur);
                if (!sortedReps.good()) {
                    std::cerr << "Error repositioning representatives file in pass 3" << std::endl;
                    return -1;
                }
            }
            
            if (outputBuffer.size() >= BUFFER_SIZE) {
                output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                           outputBuffer.size() * sizeof(TriangleCoordinates));
                outputBuffer.clear();
            }
        }
    }
    
    if (!outputBuffer.empty()) {
        output.write(reinterpret_cast<char*>(outputBuffer.data()), 
                   outputBuffer.size() * sizeof(TriangleCoordinates));
    }
    
    sortedPass2.close();
    sortedReps.close();
    output.close();
    std::remove(sortedPass2File.c_str());
    
    std::cout << "Simplified mesh created with " << nbFaces << " faces." << std::endl;
    return nbFaces;
}

int generateSimplifiedMeshBin(const std::string &representativesFilemame, const std::string &clusterFilename, const std::string &outputFilename) {
    dereferenceClusterPass1(representativesFilemame, clusterFilename, "pass1.bin");
    dereferenceClusterPass2("pass1.bin", representativesFilemame, "pass2.bin"); 
    int nbOfFaces = dereferenceClusterPass3("pass2.bin", representativesFilemame, outputFilename);

    std::remove("pass1.bin");
    std::remove("pass2.bin");

    return nbOfFaces;
}
