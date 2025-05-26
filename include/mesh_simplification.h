#ifndef MESH_SIMPLIFICATION_H
#define MESH_SIMPLIFICATION_H

#include "mesh_cutting.h"
#include "quadrics.h"
#include "structures.h"
#include <fstream>
#include <string>
#include <float.h>
#include <iostream>
#include <vector>


/**
 * @brief Computes the optimal vertex (representative) for each grid cell.
 * The data is stored in a binary file with the following format:
 *     int gridIndex (4 bytes) | Vertex optimalVertex (12 bytes)
 *
 * @param inputFilenamePlaneEquation File containing the plane equations
 * @param outputFilename Output file
 * @return File containing the optimal vertex for each grid cell
 */
bool computeGridCellRepresentatives(const std::string &inputFilenamePlaneEquation, const std::string &outputFilename);

/**
 * @brief Sorts the cluster file by the 1st index (v1) and replaces it with the corresponding vertex
 *
 * @param representativesFilename File containing the representative data
 * @param clusterFilename File containing the cluster data
 * @param outputFilename Output file
 * @return Cluster file with the 1st index replaced by the corresponding vertex
 */
bool dereferenceClusterPass1(const std::string &representativesFilename, const std::string &clusterFilename, const std::string &outputFilename);

/**
 * @brief Sorts the cluster file by the 2nd index (v2) and replaces it with the corresponding vertex
 *
 * @param representativesFilename File containing the representative data
 * @param clusterFilename File containing the cluster data
 * @param outputFilename Output file
 * @return Cluster file with the 2nd index replaced by the corresponding vertex
 */
bool dereferenceClusterPass2(const std::string &representativesFilename, const std::string &clusterFilename, const std::string &outputFilename);

/**
 * @brief Sorts the cluster file by the 3rd index (v3) and replaces it with the corresponding vertex
 *
 * @param representativesFilename File containing the representative data
 * @param clusterFilename File containing the cluster data
 * @param outputFilename Output file
 * @return Cluster file with the 3rd index replaced by the corresponding vertex
 */
int dereferenceClusterPass3(const std::string &representativesFilename, const std::string &clusterFilename, const std::string &outputFilename);

/**
 * @brief Uses 3 passes to dereference the cluster files
 *
 * @param representativesFilename File containing the representative data
 * @param clusterFilename File containing the cluster data
 * @param outputFilename Output file
 * @return Dereferenced file (Binary)
 */
int generateSimplifiedMeshBin(const std::string &representativesFilename, const std::string &clusterFilename, const std::string &outputFilename);


void externalMergeSortGridPlaneEntry(const std::string &inputFile, const std::string &outputFile);


#endif 