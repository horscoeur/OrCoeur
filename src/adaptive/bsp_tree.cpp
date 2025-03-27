#include "adaptive/bsp_tree.h"
#include "quadrics.h"
#include <string>
#include <queue>

// Default constructor for BSPNode.
BSPNode::BSPNode() : isLeaf(true), left(nullptr), right(nullptr) {
    aggregatedQuadric.data.fill(0.0f);
}

// Utility function to convert a Vertex to an Eigen::Vector3f.
Eigen::Vector3f vertexToEigen(const Vertex &v) {
    return {v.x, v.y, v.z};
}

// Computes the mean of the representatives in a vector of CellData.
Eigen::Vector3f computeMean(const std::vector<CellData>& cells) {
    Eigen::Vector3f mean = Eigen::Vector3f::Zero();
    for (const auto &cell : cells) {
        mean += vertexToEigen(cell.representative);
    }
    if (!cells.empty())
        mean /= static_cast<float>(cells.size());
    return mean;
}

// Computes the covariance (3x3) of the representatives in cells.
Eigen::Matrix3f computeCovariance(const std::vector<CellData>& cells, const Eigen::Vector3f &mean) {
    Eigen::Matrix3f cov = Eigen::Matrix3f::Zero();
    for (const auto &cell : cells) {
        Eigen::Vector3f diff = vertexToEigen(cell.representative) - mean;
        cov += diff * diff.transpose();
    }
    if (!cells.empty())
        cov /= static_cast<float>(cells.size());
    return cov;
}

// Splits a node (leaf) into two children by choosing a splitting plane based on covariance.
BSPNode* splitNode(BSPNode* node) {
    // Calculate the mean and covariance of the representatives in this node.
    const Eigen::Vector3f mean = computeMean(node->cells);
    const Eigen::Matrix3f cov = computeCovariance(node->cells, mean);

    // Compute eigenvalues and eigenvectors.
    const Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> solver(cov);
    if (solver.info() != Eigen::Success) {
        // On failure, do not split (return the node unchanged).
        return node;
    }
    // Retrieve eigenvectors (column 0 = smallest, column 2 = largest).
    const Eigen::Vector3f eig1 = solver.eigenvectors().col(0);
    const Eigen::Vector3f eig3 = solver.eigenvectors().col(2);

    // Heuristic: if the ratio between the largest and smallest eigenvalue is high,
    // choose the direction of maximum dispersion (eig3), otherwise the direction of minimum dispersion (eig1).
    const float lambdaMin = solver.eigenvalues()(0);
    const float lambdaMax = solver.eigenvalues()(2);
    Eigen::Vector3f normal;
    if (lambdaMax > 2.0f * lambdaMin)
        normal = eig3;
    else
        normal = eig1;

    // The plane passes through the mean; calculate d such that n·mean + d = 0.
    const float d = -normal.dot(mean);
    const Eigen::Vector4f splittingPlane(normal.x(), normal.y(), normal.z(), d);

    // Create the two children.
    auto* leftChild = new BSPNode();
    auto* rightChild = new BSPNode();

    // Partition the cells based on the sign of the dot product with the normal.
    for (const auto &cell : node->cells) {
        Eigen::Vector3f p = vertexToEigen(cell.representative);
        const float dist = normal.dot(p) + d;
        if (dist <= 0)
            leftChild->cells.push_back(cell);
        else
            rightChild->cells.push_back(cell);
    }

    // If one partition is empty, cancel the split to avoid empty leaves.
    if (leftChild->cells.empty() || rightChild->cells.empty()) {
        delete leftChild;
        delete rightChild;
        return node; // Do not split.
    }

    // Convert the current node to an internal node.
    node->isLeaf = false;
    node->splittingPlane = splittingPlane;
    node->left = leftChild;
    node->right = rightChild;
    // Clear the cell list of the current node (cells are now in the children).
    node->cells.clear();

    return node;
}

// Comparator for the priority queue (we want to split the node with the largest error).
bool CompareLeaf::operator()(const LeafEntry &a, const LeafEntry &b) const {
    return a.error < b.error;
}

// @brief Builds the BSP Tree until reaching the target number of leaf nodes.
BSPNode* buildBSPTree(const std::vector<CellData>& cells, const int targetLeafCount) {
    // Create the root node containing all cell representatives.
    auto* root = new BSPNode();
    root->cells = cells;

    // Extract quadrics from the root cells.
    std::vector<Quadric> quadrics;
    for (const auto &cell : cells) {
        quadrics.push_back(cell.quadric);
    }

    // Aggregate quadrics for the root and compute the optimal representative.
    root->aggregatedQuadric = addQuadrics(quadrics);
    const Vertex rootRepresentative = findOptimalVertex(root->aggregatedQuadric);

    // Calculate the root error.
    const float rootError = evaluateTotalError(quadrics, rootRepresentative);

    // Priority queue based on quadric error.
    std::priority_queue<LeafEntry, std::vector<LeafEntry>, CompareLeaf> pq;
    pq.push({root, rootError});

    int currentLeafCount = 1;
    while (currentLeafCount < targetLeafCount && !pq.empty()) {
        LeafEntry current = pq.top();
        pq.pop();
        BSPNode* node = current.node;
        // If the node is already minimal or unsplittable, skip.
        if (node->cells.size() <= 1)
            continue;

        // Split the node.
        const BSPNode* splitResult = splitNode(node);
        // If the split did not effectively partition the cells, skip.
        if (splitResult == node && node->isLeaf) {
            // Cannot effectively split this node.
            continue;
        }

        // Add the two children to the priority queue.
        if (node->left && !node->left->cells.empty()) {
            std::vector<Quadric> quadricsLeft;
            for (const auto &cell : node->left->cells) {
                quadricsLeft.push_back(cell.quadric);
            }
            // Aggregate quadrics for the left child and compute the optimal representative.
            node->left->aggregatedQuadric = addQuadrics(quadricsLeft);
            const Vertex leftRepresentative = findOptimalVertex(node->left->aggregatedQuadric);
            const float errorLeft = evaluateTotalError(quadricsLeft, leftRepresentative);
            pq.push({node->left, errorLeft});
        }
        if (node->right && !node->right->cells.empty()) {
            std::vector<Quadric> quadricsRight;
            for (const auto &cell : node->right->cells) {
                quadricsRight.push_back(cell.quadric);
            }
            // Aggregate quadrics for the right child and compute the optimal representative.
            node->right->aggregatedQuadric = addQuadrics(quadricsRight);
            const Vertex rightRepresentative = findOptimalVertex(node->right->aggregatedQuadric);
            const float errorRight = evaluateTotalError(quadricsRight, rightRepresentative);
            pq.push({node->right, errorRight});
        }
        currentLeafCount++;  // Increase leaf count by at least one.
    }

    return root;
}

/**
 * @brief Plots the BSP Tree in Polyscope.
 *
 * See documentation in the header file.
 */
void plotBSP(std::vector<polyscope::PointCloud *>& point_clouds, BSPNode *node, const float depth) {
    if (node->isLeaf) {
        std::vector<std::array<double, 3>> localPoints;
        for (const auto &cell : node->cells) {
            localPoints.push_back({cell.representative.x, cell.representative.y, cell.representative.z});
        }
        std::string name = "Leaf " + std::to_string(node->cells.size()) + " cells: (";
        for (const auto &cell : node->cells) {
            name += std::to_string(cell.cellIndex) + " ";
        }
        name += ")";
        point_clouds.push_back(polyscope::registerPointCloud(name, localPoints));
    } else {
        plotBSP(point_clouds, node->left, depth + 1);
        plotBSP(point_clouds, node->right, depth + 1);
    }
}
