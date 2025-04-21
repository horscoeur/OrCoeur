
#include "mesh_cutting.h"

bool meshCutting(const std::string &inFilenameBinary, const std::string &outputFilenamePlaneEquation, const std::string &outputFilenameTriangleCluster, int resolution, bool cutTheMesh) {
    // Open the input file
    std::ifstream file(inFilenameBinary, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << inFilenameBinary << ".\n";
        return false;
    }

    // Pass the header
    std::string line;
    bool header = true;
    while (header && std::getline(file, line)) {
        if (line.find("END_HEADER") != std::string::npos) {
            header = false;
        }
    }

    // Get the position of the end of the header
    std::streampos pos = file.tellg();
    if (pos == -1) {
        std::cerr << "Error : Could not find the end of the header.\n";
        file.close();
        return false;
    }


    // Open the output files
    std::ofstream planeEquation(outputFilenamePlaneEquation);
    std::ofstream triangleCluster(outputFilenameTriangleCluster);
    if (!planeEquation.is_open() || !triangleCluster.is_open()) {
        std::cerr << "Error: Could not open output files.\n";
        file.close();
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

    // If we only want to display the grid and not cut the mesh, close the files and return true
    if (!cutTheMesh) {
        grid.displayGrid();
        file.close();
        planeEquation.close();
        triangleCluster.close();
        return true;
    }

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
        PlaneEquation nt = computePlaneEquation(face);

        // Write the plane equation to the output file for each vertex
        planeEquation.write(reinterpret_cast<const char*>(&v1Index), sizeof(int));
        planeEquation.write(reinterpret_cast<const char*>(&nt), sizeof(PlaneEquation));
        planeEquation.write(reinterpret_cast<const char*>(&v2Index), sizeof(int));
        planeEquation.write(reinterpret_cast<const char*>(&nt), sizeof(PlaneEquation));
        planeEquation.write(reinterpret_cast<const char*>(&v3Index), sizeof(int));
        planeEquation.write(reinterpret_cast<const char*>(&nt), sizeof(PlaneEquation));

        // Write the triangle to the output file only if each vertex is in a different cell
        if (v1Index != v2Index && v2Index != v3Index && v3Index != v1Index) {
            triangleCluster.write(reinterpret_cast<const char*>(&v1Index), sizeof(int));
            triangleCluster.write(reinterpret_cast<const char*>(&v2Index), sizeof(int));
            triangleCluster.write(reinterpret_cast<const char*>(&v3Index), sizeof(int));
        }

    }

    // Close the files and return true
    file.close();
    planeEquation.close();
    triangleCluster.close();
    return true;
}


