#ifndef MESH_CUTTING_H
#define MESH_CUTTING_H

#include "mesh_cutting.h"
#include "structures.h"
#include <fstream>
#include <string>
#include <float.h>
#include <iostream>


/**
 * @brief Computes the cross product of two vertices.
 *
 * This function computes the cross product of two vertices.
 *
 * @param a First vertex.
 * @param b Second vertex.
 * @return The cross product of the two vertices.
 */
Vertex crossProduct(const Vertex& a, const Vertex& b);

/**
 * @brief Computes the scalar triple product of three vertices.
 *
 * This function computes the scalar triple product of three vertices.
 *
 * @param a First vertex.
 * @param b Second vertex.
 * @param c Third vertex.
 * @return The scalar triple product of the three vertices.
 */
float scalarTripleProduct(const Vertex& a, const Vertex& b, const Vertex& c);

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
 * @return True if the mesh was successfully cut, false otherwise.
 */
bool meshCutting(const std::string &filename, std::string &outputFilenamePlaneEquation, std::string &outputFilenameTriangleCluster, int resolution);

#endif //MESH_CUTTING_H
