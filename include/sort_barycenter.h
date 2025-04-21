#ifndef SORT_BARYCENTER_H
#define SORT_BARYCENTER_H

#include "structures.h"
#include <string>
#include <vector>
#include <fstream>
#include <queue>

// Maximum number of triangles loaded in memory to sort a chunk
#define CHUNK_SIZE 1000000 // 12 MB for TriangleCoordinates

/**
 * @brief Calculates the barycenter of a triangle from its three vertices.
 * 
 * The barycenter is the center of gravity of the triangle, calculated as the average
 * of the coordinates of the three vertices.
 * 
 * @param triangle The coordinates of the triangle
 * @return Vertex The barycenter of the triangle
 */
Vertex calculateBarycenter(const TriangleCoordinates& triangle);

/**
 * @brief Sorts a batch (chunk) of triangles by their barycenter.
 * 
 * This function sorts triangles in memory using a standard sort.
 * The sort is performed according to the specified axis (x, y, or z).
 * 
 * @param triangles The vector of triangles with barycenters to sort
 * @param sortAxis The axis to sort by ('x', 'y', or 'z')
 */
void sortChunk(std::vector<TriangleWithBarycenter>& triangles, char sortAxis);

/**
 * @brief Structure representing chunk information for merge sort.
 * 
 * This structure stores a triangle with its barycenter and the corresponding
 * source file to facilitate reading the next triangle.
 */
struct ChunkInfo {
    TriangleWithBarycenter triangleWithBarycenter;
    std::ifstream* file;
    char sortAxis;
    
    /**
     * @brief Constructor with default sort axis.
     * 
     * @param axis Sort axis ('x', 'y', or 'z')
     */
    ChunkInfo(char axis = 'x') : sortAxis(axis) {}
    
    /**
     * @brief Comparison operator for the priority queue.
     * 
     * Compares triangles by their barycenter on the specified axis.
     * 
     * @param other The other ChunkInfo to compare with
     * @return bool true if this triangle has a larger barycenter than the other
     */
    bool operator>(const ChunkInfo& other) const;
};

/**
 * @brief Sorts triangles by their barycenter using an out-of-core approach.
 * 
 * This function sorts large quantities of triangles that cannot fit entirely
 * in memory. It uses a two-phase approach:
 * 1. Divide triangles into chunks, calculate barycenters, sort each chunk
 * 2. Merge the sorted chunks into a single output file
 * 
 * The sort is performed according to the specified axis (x, y, or z of the barycenter).
 * 
 * @param inputFilename Path to the input file containing unsorted triangles
 * @param outputFilename Path to the output file where sorted triangles will be written
 * @param sortAxis Axis to sort by ('x', 'y', or 'z')
 * @return bool true if sorting succeeds, false otherwise
 */
bool sortTrianglesByBarycenter(const std::string &inputFilename, const std::string &outputFilename, char sortAxis);

#endif // SORT_BARYCENTER_H