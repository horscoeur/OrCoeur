
#include "mesh_cutting.h"
#include <vector>

#include "polyscope/point_cloud.h"

Vertex crossProduct(const Vertex& a, const Vertex& b) {
    Vertex result{};
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    std :: cout << "Cross Product: " << result.x << " " << result.y << " " << result.z << " Between " << a.x << " " << a.y << " " << a.z << " and " << b.x << " " << b.y << " " << b.z << std :: endl;
    return result;
}

float scalarTripleProduct(const Vertex& a, const Vertex& b, const Vertex& c) {
    return a.x * (b.y * c.z - b.z * c.y) -
           a.y * (b.x * c.z - b.z * c.x) +
           a.z * (b.x * c.y - b.y * c.x);
}

bool meshCutting(const std::string &inFilenameBinary, std::string &outputFilenamePlaneEquation, std::string &outputFilenameTriangleCluster, int resolution){
    // Open the input file
    std::ifstream file(inFilenameBinary, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << inFilenameBinary << ".\n";
        return false;
    }

    // remove the header
    std::string line;
    bool header = true;
    while (header && std::getline(file, line)) {
        if (line.find("END_HEADER") != std::string::npos) {
            header = false;
        }
    }

    // Vérifier la position après l'en-tête
    std::streampos pos = file.tellg();
    std::cout << "Position après l'en-tête: " << pos << std::endl;

    if (pos == -1) {
        std::cerr << "Erreur : Position invalide après l'en-tête !" << std::endl;
        file.clear(); // Réinitialise les erreurs
    }


    // Open the output files
    std::ofstream planeEquation(outputFilenamePlaneEquation);
    std::ofstream triangleCluster(outputFilenameTriangleCluster);
    if (!planeEquation.is_open() || !triangleCluster.is_open()) {
        std::cerr << "Error: Could not open output files.\n";
        return false;
    }
    // Create the grid and the face
    Grid grid(resolution);
    TriangleCoordinates face{};

    // First pass to get the min and max of the mesh
    grid.min = {FLT_MAX, FLT_MAX, FLT_MAX};
    grid.max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};
    while (file.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))){
        grid.min.x = std::min(grid.min.x, std::min(face.v1.x, std::min(face.v2.x, face.v3.x)));
        grid.min.y = std::min(grid.min.y, std::min(face.v1.y, std::min(face.v2.y, face.v3.y)));
        grid.min.z = std::min(grid.min.z, std::min(face.v1.z, std::min(face.v2.z, face.v3.z)));

        grid.max.x = std::max(grid.max.x, std::max(face.v1.x, std::max(face.v2.x, face.v3.x)));
        grid.max.y = std::max(grid.max.y, std::max(face.v1.y, std::max(face.v2.y, face.v3.y)));
        grid.max.z = std::max(grid.max.z, std::max(face.v1.z, std::max(face.v2.z, face.v3.z)));
    }

    grid.min = grid.min - Vertex(0.1,0.1,0.1);
    grid.max = grid.max + Vertex(0.1,0.1,0.1);

    // Rewind the file after the header
    file.clear();
    file.seekg(pos);

    // Second pass to write the plane equation and triangle cluster
    while (file.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))) {
        Vertex v1 = face.v1;
        Vertex v2 = face.v2;
        Vertex v3 = face.v3;

        // Get the indices of the vertices in the grid
        int v1Index = grid.getIndex(v1);
        int v2Index = grid.getIndex(v2);
        int v3Index = grid.getIndex(v3);

        // Compute the nt of the triangle
        Vertex crossV1V2 = crossProduct(v1, v2);
        Vertex crossV2V3 = crossProduct(v2, v3);
        Vertex crossV3V1 = crossProduct(v3, v1);
        float productScalar = scalarTripleProduct(v1, v2, v3);

        Vertex4 nt = crossV1V2 + crossV2V3 + crossV3V1;
        nt.w = -productScalar;

        // Write the plane equation to the output file for each vertex
        planeEquation << v1Index << " " << nt.toString() << "\n";
        planeEquation << v2Index << " " << nt.toString() << "\n";
        planeEquation << v3Index << " " << nt.toString() << "\n";

        // Write the triangle to the output file only if each vertex is in a different cell
        if (v1Index != v2Index && v2Index != v3Index && v3Index != v1Index) {
            triangleCluster << v1Index << " " << v2Index << " " << v3Index << "\n";
        }

    }

    // Close the files and return true
    file.close();
    planeEquation.close();
    triangleCluster.close();
    return true;
}


