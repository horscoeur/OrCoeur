#ifndef ADAPTATIVE_MESH_CUTTING_H
#define ADAPTATIVE_MESH_CUTTING_H

#include "structures.h"
#include <string>
#include <vector>

/**
 * @brief Computes the dual quadric of a vertex.
 *
 * For a vertex v, the dual quadric is defined as:
 *    D = v * v^T,   e = v,   f = 1.
 *
 * @param vertex The vertex to process.
 * @return DualQuadric The computed dual quadric.
 */
DualQuadric computeDualQuadric(const Vertex &vertex);

/**
 * @brief Adds two dual quadrics.
 *
 * The addition is performed component-wise (matrix, vector, and scalar).
 *
 * @param dq1 First dual quadric.
 * @param dq2 Second dual quadric.
 * @return DualQuadric The resulting dual quadric.
 */
DualQuadric addDualQuadric(const DualQuadric &dq1, const DualQuadric &dq2);

/**
 * @brief Computes the representative vertex from an accumulated dual quadric.
 *
 * The optimal vertex is computed here as v_opt = e / f.
 *
 * @param dq The accumulated dual quadric.
 * @return Vertex The representative vertex.
 */
Vertex computeRepresentativeFromDualQuadric(const DualQuadric &dq);

/**
 * @brief Performs the dual quadric quantization of a mesh for subsequent BSP Tree construction.
 *
 * This function processes a binary input file containing the triangle coordinates of a mesh.
 * It performs the following steps:
 *   - First pass: Reads the mesh to determine its bounding box.
 *   - Expands the bounding box by a small margin.
 *   - If cutTheMesh is false, displays the grid (useful for debugging) and exits.
 *   - Second pass: For each triangle, it:
 *       - Retrieves its vertices and computes the corresponding grid cell indices.
 *       - Computes and accumulates both the quadrics and dual quadrics for each cell.
 *       - Writes the triangle's cell indices to a cluster file if the vertices belong to different cells.
 *   - Finally, iterates over the accumulated data to compute, for each occupied cell,
 *     the optimal representative (using findOptimalVertex on the aggregated quadric),
 *     and stores the cell data (cell index, representative, dual quadric, and quadric) in a vector.
 *
 * @param inFilenameBinary Name of the input binary file containing the mesh.
 * @param resolution Grid resolution (number of cells per dimension).
 */
std::vector<CellData> meshCuttingDualQuadric(const std::string &inFilenameBinary, int resolution);

#endif // ADAPTATIVE_MESH_CUTTING_H