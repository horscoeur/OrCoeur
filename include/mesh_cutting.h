#ifndef MESH_CUTTING_H
#define MESH_CUTTING_H

#include "mesh_cutting.h"
#include "structures.h"
#include <fstream>
#include <string>
#include <float.h>
#include <iostream>
#include <vector>
#include "quadrics.h"
#include "polyscope/point_cloud.h"


/**
 * @brief Cuts a mesh into clusters and writes his plane equation and triangle clusters to separate files.
 *
 * This function reads a binary soup file and cuts it into clusters and writes the plane equation
 * and triangle clusters to separate files.
 *
 * @param filename Input binary Soup file path.
 * @param outputFilenamePlaneEquation Output text file path for the plane equation.
 * @param outputFilenameTriangleCluster Output text file path for the triangle clusters.
 * @param resolution Resolution of the cutting.
 * @param cutTheMesh If true, the mesh will be cut, otherwise only the grid will be displayed.
 * @return True if the mesh was successfully cut, false otherwise.
 */
Grid meshCutting(const std::string &filename, const std::string &outputFilenamePlaneEquation, const std::string &outputFilenameTriangleCluster, int resolution, bool cutTheMesh = true);

#endif //MESH_CUTTING_H
