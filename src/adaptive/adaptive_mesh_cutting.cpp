#include "adaptive/adaptive_mesh_cutting.h"
#include "quadrics.h"

#include <fstream>
#include <iostream>
#include <cfloat>
#include <algorithm>
#include <unordered_map>

DualQuadric computeDualQuadric(const Vertex &vertex) {
    DualQuadric dq;
    // Initialize the matrix D (3x3 stored linearly)
    dq.D[0] = vertex.x * vertex.x;  dq.D[1] = vertex.x * vertex.y;  dq.D[2] = vertex.x * vertex.z;
    dq.D[3] = vertex.y * vertex.x;  dq.D[4] = vertex.y * vertex.y;  dq.D[5] = vertex.y * vertex.z;
    dq.D[6] = vertex.z * vertex.x;  dq.D[7] = vertex.z * vertex.y;  dq.D[8] = vertex.z * vertex.z;

    // The vector e and scalar f
    dq.e = vertex;
    dq.f = 1.0f;

    return dq;
}

DualQuadric addDualQuadric(const DualQuadric &dq1, const DualQuadric &dq2) {
    DualQuadric res;
    for (int i = 0; i < 9; ++i) {
        res.D[i] = dq1.D[i] + dq2.D[i];
    }
    res.e.x = dq1.e.x + dq2.e.x;
    res.e.y = dq1.e.y + dq2.e.y;
    res.e.z = dq1.e.z + dq2.e.z;
    res.f = dq1.f + dq2.f;
    return res;
}

Vertex computeRepresentativeFromDualQuadric(const DualQuadric &dq) {
    if (dq.f != 0.0f)
        return { dq.e.x / dq.f, dq.e.y / dq.f, dq.e.z / dq.f };
    else
        return { 0.0f, 0.0f, 0.0f };
}

std::vector<CellData> meshCuttingDualQuadric(const std::string &inFilenameBinary, int resolution) {

    std::vector<CellData> cells;

    // Open the input file
    std::ifstream file(inFilenameBinary, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << inFilenameBinary << ".\n";
        return {};
    }

    // Skip header lines
    std::string line;
    bool header = true;
    while (header && std::getline(file, line)) {
        if (line.find("END_HEADER") != std::string::npos) {
            header = false;
        }
    }

    // Get the end-of-header position
    std::streampos pos = file.tellg();
    if (pos == -1) {
        std::cerr << "Error: Could not find the end of the header.\n";
        file.close();
        return {};
    }

    // Create the grid
    Grid grid(resolution);
    TriangleCoordinates face{};

    // First pass: compute the mesh bounding box
    grid.min = { FLT_MAX, FLT_MAX, FLT_MAX };
    grid.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
    while (file.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))) {
        grid.min.x = std::min(grid.min.x, std::min(face.v1.x, std::min(face.v2.x, face.v3.x)));
        grid.min.y = std::min(grid.min.y, std::min(face.v1.y, std::min(face.v2.y, face.v3.y)));
        grid.min.z = std::min(grid.min.z, std::min(face.v1.z, std::min(face.v2.z, face.v3.z)));

        grid.max.x = std::max(grid.max.x, std::max(face.v1.x, std::max(face.v2.x, face.v3.x)));
        grid.max.y = std::max(grid.max.y, std::max(face.v1.y, std::max(face.v2.y, face.v3.y)));
        grid.max.z = std::max(grid.max.z, std::max(face.v1.z, std::max(face.v2.z, face.v3.z)));
    }

    // Expand the bounding box by a small margin
    grid.min = grid.min - Vertex(0.1f, 0.1f, 0.1f);
    grid.max = grid.max + Vertex(0.1f, 0.1f, 0.1f);

    // Reset the stream to the end of the header
    file.clear();
    file.seekg(pos);

    // Accumulation tables: for each cell (index), accumulate the dual quadric and the quadric
    std::unordered_map<int, DualQuadric> cellDuals;
    std::unordered_map<int, Quadric> cellQuadrics;

    // Second pass: for each triangle, perform accumulation and write triangle clusters
    while (file.read(reinterpret_cast<char *>(&face), sizeof(TriangleCoordinates))) {
        // Retrieve the vertices
        Vertex v1 = face.v1;
        Vertex v2 = face.v2;
        Vertex v3 = face.v3;

        // Get the cell indices for each vertex
        int v1Index = grid.getIndex(v1);
        int v2Index = grid.getIndex(v2);
        int v3Index = grid.getIndex(v3);

        // Compute and accumulate quadrics for each cell
        Quadric q1 = computeQuadric(face);
        Quadric q2 = computeQuadric(face);
        Quadric q3 = computeQuadric(face);

        cellQuadrics[v1Index] = cellQuadrics.contains(v1Index) ? addQuadric(cellQuadrics[v1Index], q1) : q1;
        cellQuadrics[v2Index] = cellQuadrics.contains(v2Index) ? addQuadric(cellQuadrics[v2Index], q2) : q2;
        cellQuadrics[v3Index] = cellQuadrics.contains(v3Index) ? addQuadric(cellQuadrics[v3Index], q3) : q3;

        // Compute and accumulate dual quadrics for each cell
        DualQuadric dq1 = computeDualQuadric(v1);
        DualQuadric dq2 = computeDualQuadric(v2);
        DualQuadric dq3 = computeDualQuadric(v3);

        cellDuals[v1Index] = cellDuals.contains(v1Index) ? addDualQuadric(cellDuals[v1Index], dq1) : dq1;
        cellDuals[v2Index] = cellDuals.contains(v2Index) ? addDualQuadric(cellDuals[v2Index], dq2) : dq2;
        cellDuals[v3Index] = cellDuals.contains(v3Index) ? addDualQuadric(cellDuals[v3Index], dq3) : dq3;
    }

    // Iterate over the accumulation table to compute the representative of each cell
    // and record the accumulated dual quadric
    for (const auto &entry : cellDuals) {
        int cellIndex = entry.first;
        const DualQuadric &dq = entry.second;
        const Quadric &q = cellQuadrics[cellIndex];
        Vertex representative = findOptimalVertex(q);

        // Append the cell data: (cellIndex, representative, dual quadric, quadric)
        cells.emplace_back(CellData{cellIndex, representative, dq, q});
    }

    // Close the files
    file.close();
    return cells;
}