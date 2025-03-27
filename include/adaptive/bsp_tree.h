#ifndef BSP_TREE_H
#define BSP_TREE_H

#include "structures.h"
#include <vector>
#include <Eigen/Dense>

// Structure of a BSP Tree node.
struct BSPNode {
    bool isLeaf;
    Eigen::Vector4f splittingPlane;
    BSPNode* left;
    BSPNode* right;
    Quadric aggregatedQuadric;

    // For a leaf, store the associated cells.
    std::vector<CellData> cells;

    BSPNode();
};

// Utility function to convert a Vertex to an Eigen::Vector3f.
Eigen::Vector3f vertexToEigen(const Vertex &v);

// Computes the mean of the representatives in a vector of CellData.
Eigen::Vector3f computeMean(const std::vector<CellData>& cells);

// Computes the covariance (3x3) of the representatives in cells.
Eigen::Matrix3f computeCovariance(const std::vector<CellData>& cells, const Eigen::Vector3f &mean);

/**
 * @brief Splits a node (leaf) into two children by choosing a splitting plane based on covariance.
 *
 * This function performs the following steps:
 *   - Computes the mean and covariance of the representatives in the node.
 *   - Calculates the eigenvalues and eigenvectors of the covariance matrix.
 *   - Chooses the splitting direction using a heuristic comparing the smallest and largest eigenvalues.
 *   - Creates two children and partitions the cells based on the splitting plane.
 *   - If one partition is empty, the split is cancelled to avoid empty leaves.
 *
 * @param node Pointer to the BSPNode to be split.
 * @return BSPNode* Pointer to the node after attempting to split.
 */
BSPNode* splitNode(BSPNode* node);

// Structure used in the priority queue (using number of cells as an error indicator).
struct LeafEntry {
    BSPNode* node;
    float error;
};

// Comparator for the priority queue (we want to split the node with the largest error).
struct CompareLeaf {
    bool operator()(const LeafEntry &a, const LeafEntry &b) const;
};

/**
 * @brief Builds the BSP Tree until reaching the target number of leaf nodes.
 *
 * This function:
 *   - Creates a root node containing all cell representatives.
 *   - Aggregates the quadrics and computes an optimal representative for the root.
 *   - Uses a priority queue based on quadric error to iteratively select and split nodes.
 *
 * @param cells Vector containing cell data (index, representative, dual quadric, and quadric).
 * @param targetLeafCount Desired number of leaves.
 * @return BSPNode* Pointer to the root of the constructed BSP Tree.
 */
BSPNode* buildBSPTree(const std::vector<CellData>& cells, int targetLeafCount);

/**
 * @brief Plots the BSP Tree in Polyscope.
 *
 * This function recursively plots the points for leaf nodes using Polyscope.
 *
 * @param point_clouds Vector to store Polyscope point clouds.
 * @param node The root of the BSP Tree.
 * @param depth The depth of the current node in the tree.
 */
void plotBSP(std::vector<polyscope::PointCloud *>& point_clouds, BSPNode *node, float depth);

#endif // BSP_TREE_H