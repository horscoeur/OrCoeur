#ifndef STREAM_SIMPLIFICATION_H
#define STREAM_SIMPLIFICATION_H

#include "structures.h"


struct StreamMeshData {
    // Map to store the vertex coordinates and their corresponding indices
    std::map<Vertex, int> vertexMap;
    int actual_unique_id = 0;

    // Map to store the adjacency list of each vertex
    std::map<int, std::vector<int>> adjacencyList;
    // Map to store the adjacency list of each vertex that has already been seen
    std::map<int, std::vector<int>> adjacencyListAlreadySeen;
    // Map to store the triangle coordinates and their corresponding indices
    std::map<int, std::vector<int>> triangleList;
    // Map to store the vertex indices that are not in the border
    std::vector<int> vertexNotInBorder;

    // Map to store if the vertex has been simplified
    std::map<int, bool> vertexSimplified;

    // Map to store the triangle coordinates in core memory
    std::map<int,TriangleCoordinates> inCoreTriangleBuffer;
    // Map to store the quadric error metric for each triangle
    std::map<int, Quadric> triangleQuadricMap;
    // Both of these maps use this unique index to identify triangles
    int unique_triangle_index = 0;

    StreamMeshData() {
        vertexMap = std::map<Vertex, int>();
        actual_unique_id = 0;
        adjacencyList = std::map<int, std::vector<int>>();
        adjacencyListAlreadySeen = std::map<int, std::vector<int>>();
        vertexNotInBorder = std::vector<int>();
        triangleList = std::map<int, std::vector<int>>();
        triangleQuadricMap = std::map<int, Quadric>();
        unique_triangle_index = 0;
        vertexSimplified = std::map<int, bool>();
    }
};



bool streamSimplificationVisualization(const std::string &inputFile, const std::string &outputFile, int maxTrianglesInBuffer, float decimationPercentage, bool visualizeSimplification);



/**
 * @brief Track neighbor visits and update border status in out-of-core streaming.
 *
 * When processing a neighbor for a vertex:
 *   1. If it’s the **first** time seeing `neighborId` for `vertexId`:
 *      - Insert `neighborId` into `meshData.adjacencyList[vertexId]` (kept sorted).
 *   2. If it’s the **second** time (already in `adjacencyList`):
 *      - Remove `neighborId` from `meshData.adjacencyList[vertexId]`.
 *      - Insert `neighborId` into `meshData.adjacencyListAlreadySeen[vertexId]` (kept sorted).
 *      - If `adjacencyList[vertexId]` is now empty, add `vertexId` to `meshData.vertexNotInBorder`.
 *
 * @param vertexId   ID of the vertex whose neighbors are being updated.
 * @param neighborId ID of the neighbor being added or removed.
 * @param meshData   StreamMeshData containing:
 *                     - `adjacencyList`            : active neighbor lists,
 *                     - `vertexNotInBorder`        : vertices with no remaining live neighbors,
 *                     - `adjacencyListAlreadySeen` : lists of neighbors seen twice.
 */
void updateAdjacency(int vertexId, int neighborId, StreamMeshData& meshData);



bool read(std::ifstream & inputFile, int numberToRead,StreamMeshData &meshData);

/**
 * @brief Merge and update the “already seen” adjacency lists when collapsing an edge,
 *        and remove any degenerate triangles formed in the process.
 *
 * When collapsing the edge between vertexA and vertexB:
 *   1. Merge the two sorted adjacencyListAlreadySeen entries for vertexA and vertexB
 *      into a single sorted list for newIndice, dropping any reference to oldIndice.
 *   2. For each neighbor in the merged list, replace occurrences of oldIndice with newIndice
 *      in that neighbor’s adjacencyListAlreadySeen.
 *   3. Identify and remove the degenerate triangle shared by vertexA, vertexB, and each
 *      common neighbor (triangle whose three vertices collapse onto two points):
 *      - Erase that triangle from inCoreTriangleBuffer.
 *      - Remove its index from triangleList for vertexA, vertexB, and the common neighbor.
 *   4. Erase the oldIndice entry from meshData.adjacencyListAlreadySeen and
 *      assign the merged list to meshData.adjacencyListAlreadySeen[newIndice].
 *
 * @param meshData      The mesh state containing adjacency and triangle data.
 * @param vertexA       One endpoint index of the collapsing edge.
 * @param vertexB       The other endpoint index of the collapsing edge.
 * @param newIndice     The index to keep (usually min(vertexA, vertexB)).
 * @param oldIndice     The index to remove (the other endpoint).
 */
void decimatePartAdjacencyListAlreadySeen(StreamMeshData& meshData, int vertexA, int vertexB, int newIndice, int oldIndice);


/**
 * @brief Retrieve the 3D coordinates of two existing vertices and the new merged vertex.
 *
 * Looks up vertexA and vertexB in meshData.vertexMap to obtain their coordinates, and
 * also retrieves the coordinate for newIndice (currently just read from the map).
 *
 * @note  In the future, compute coordNewIndice from the quadric error metric rather
 *        than simply reading it from the map (TODO).
 *
 * @param meshData        The mesh data containing the vertexMap (Vertex -> index).
 * @param vertexA         The index of the first vertex being collapsed.
 * @param vertexB         The index of the second vertex being collapsed.
 * @param newIndice       The index of the merged vertex to keep.
 * @param[out] coordVertexA   Filled with the 3D coordinate of vertexA.
 * @param[out] coordVertexB   Filled with the 3D coordinate of vertexB.
 * @param[out] coordNewIndice Filled with the 3D coordinate of newIndice.
 */
void findCoordInVertexMap(StreamMeshData& meshData,int vertexA,int vertexB,int newIndice,Vertex& coordVertexA,Vertex& coordVertexB,Vertex& coordNewIndice);


/**
 * @brief Update triangle connectivity and coordinates when collapsing an edge.
 *
 * This function merges the triangle lists of two vertices (vertexA, vertexB) into a single
 * list for newIndice, updating each triangle’s vertex coordinate to coordNewIndice, and
 * ensures that no degenerate triangle (one that would collapse to a line) remains.
 *
 * Steps performed:
 *  1. Traverse the sorted triangle index lists for vertexA and vertexB in lock-step.
 *     - If a triangle appears in both lists, it indicates a shared face that has become
 *       degenerate by this collapse; remove it from inCoreTriangleBuffer and from all
 *       three per-vertex triangle lists.
 *     - Otherwise, take the smaller index, fetch its TriangleCoordinates, replace the
 *       occurrence of coordVertexA or coordVertexB with coordNewIndice, and add that
 *       triangle index to trianglesToNewIndice.
 *  2. After the main loop, process any remaining triangles on either side in the same way.
 *  3. Erase the oldIndice entry from meshData.triangleList, and assign the merged,
 *     updated list to meshData.triangleList[newIndice].
 *
 * @param meshData        The mesh state containing triangleList and inCoreTriangleBuffer.
 * @param vertexA         Index of the first vertex being collapsed.
 * @param vertexB         Index of the second vertex being collapsed.
 * @param newIndice       The vertex index to keep (merged result).
 * @param oldIndice       The vertex index to remove.
 * @param coordVertexA    3D coordinates of vertexA (before collapse).
 * @param coordVertexB    3D coordinates of vertexB (before collapse).
 * @param coordNewIndice  3D coordinates of the merged vertex (after collapse).
 */
void decimatePartTriangle(StreamMeshData& meshData,int vertexA,int vertexB,int newIndice,int oldIndice,Vertex& coordVertexA,Vertex& coordVertexB,Vertex& coordNewIndice);

/**
 * @brief Collapse the edge between two vertices and update all mesh data structures.
 *
 * This function performs an edge collapse of the mesh by merging vertexA and vertexB into a
 * single vertex (newIndice), removing the other (oldIndice), and updating:
 *   1. adjacencyList: erase oldIndice since it has no remaining “live” neighbors.
 *   2. adjacencyListAlreadySeen: call decimatePartAdjacencyListAlreadySeen to merge and reindex
 *      the “already seen” neighbor lists (keeping them sorted) and remove any degenerate triangles.
 *   3. triangleQuadricMap: combine the two per-vertex quadrics into newIndice’s quadric, then erase oldIndice.
 *   4. triangleList & inCoreTriangleBuffer: call decimatePartTriangle to replace occurrences of
 *      vertexA/vertexB with newIndice in each triangle, merge their triangle lists, and remove
 *      any degenerate triangles.
 *   5. vertexMap: remove the two old vertex coordinates, then insert the coordinate for newIndice.
 *   6. vertexNotInBorder: remove oldIndice from the list of collapsible vertices.
 *
 * @warning  ALL adjacency lists must remain sorted for the merge-based algorithms in
 *           decimatePartAdjacencyListAlreadySeen and decimatePartTriangle to work correctly.
 *
 * @param vertexA    Index of the first endpoint of the edge to collapse.
 * @param vertexB    Index of the second endpoint of the edge to collapse.
 * @param meshData   Reference to the mesh data (adjacency lists, triangle data, quadric map, etc.).
 * @return           true on success, false if any step fails (e.g., missing coordinates).
 */
bool decimateThisEdge(int vertexA, int vertexB, Vertex & position, StreamMeshData& meshData);

bool decimateThisEdge(int vertexA, int vertexB, StreamMeshData& meshData);

bool decimate(int numberToDecimate, StreamMeshData &meshData);// TODO CHange that

bool write(std::ofstream &outputFile, StreamMeshData &meshData, int numberToWrite, int *trianglesWritten);

bool initBuffer(std::ifstream& inputFile, int numberToRead, float decimationPercentage,StreamMeshData &meshData);

void displayFromBuffer(std::vector<TriangleCoordinates> & inCoreTriangleBuffer);

void displayBorderTriangles(StreamMeshData &meshData);

bool isCollapseValid(int vertexA, int vertexB, Vertex &positionAfterCollapse, StreamMeshData &meshData);


int getRandomNeighborNotInBorder(StreamMeshData &meshData);

#endif