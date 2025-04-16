#ifndef ADAPTIVE_MESH_SIMPLIFICATION_H
#define ADAPTIVE_MESH_SIMPLIFICATION_H

#include "bsp_tree.h"
#include "quadrics.h"

/*
 * @brief Resets the aggregated quadric of each node in the BSP tree.
 *
 * @param node Pointer to the root of the BSP tree.
 */
void resetAggregatedQuadric(BSPNode *node);

/**
 * @brief Computes the optimal representative vertex for each leaf node in the BSP tree.
 *
 * @param representativeVertices Vector to store the computed representative vertices.
 * @param node Pointer to the current node in the BSP tree.
 */
void computeRepresentative(std::vector<Vertex> &representativeVertices, BSPNode *node);

/**
 * @brief Distributes the quadrics of the faces to the aggregated quadrics of the BSP tree leaves.
 *
 * @param file Reference to the input file stream.
 * @param dataStart Position in the file where the face data starts.
 * @param root Pointer to the root of the BSP tree.
 */
void distributeQuadrics(std::ifstream &file, std::streampos dataStart, BSPNode *root);

/**
 * @brief Second pass: generate simplified mesh using the computed representatives
 *
 * @param file Reference to the input file stream.
 * @param outputFile Reference to the output file stream.
 * @param dataStart Position in the file where the face data starts.
 * @param root Pointer to the root of the BSP tree.
 * @return The number of faces written to the output file.
 */
int writeSimplifiedMesh(std::ifstream &file, std::ofstream &outputFile, std::streampos dataStart, BSPNode *root);

/**
 * @brief Performs adaptive mesh simplification using a BSP tree and quadric error metrics.
 *
 * This function reads a binary file containing triangle coordinates of a mesh, and simplifies the mesh
 * by clustering geometry using a BSP tree. The simplification is performed in three main stages:
 *
 *   1. First pass on the faces:
 *      - For each triangle, the BSP tree is traversed to find the leaf node containing each vertex.
 *      - The face quadric is computed and distributed to the aggregated quadrics of the three corresponding leaves.
 *
 *   2. Computing representatives:
 *      - For each leaf node of the BSP tree, an optimal representative vertex is computed
 *        using the aggregated quadric (via `findOptimalVertex`).
 *      - These representatives will be used to build the simplified mesh.
 *
 *   3. Second pass on the faces:
 *      - The input file is read again. For each triangle:
 *          - The leaf nodes of its vertices are retrieved.
 *          - If the three vertices do not fall into three distinct leaves, the face is considered degenerate and ignored.
 *          - Otherwise, each vertex is replaced by its leaf's representative, and the new face is written to the output.
 *
 * The output file starts with a simple ASCII header indicating the number of vertices and faces,
 * followed by the simplified mesh written in binary format.
 *
 * @param inputFacesFilenameBinary Path to the input binary file containing the triangle mesh.
 * @param outputFilenameBinary Path where the simplified mesh will be saved.
 * @param root Pointer to the root of the BSP tree used for spatial clustering and simplification.
 */
void adaptiveMeshSimplification(const std::string &inputFacesFilenameBinary, const std::string &outputFilenameBinary, BSPNode *root);

#endif //ADAPTIVE_MESH_SIMPLIFICATION_H
