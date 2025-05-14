#ifndef QUADRIC_CALCULATOR_H
#define QUADRIC_CALCULATOR_H

#include "structures.h"
#include <array>
#include <vector>
#include <cmath>
#include <Eigen/Dense>


/**
 * @brief Computes the plane equation (homogeneous vector) for a given triangle.
 *
 * This function calculates the triangle's normal using the cross product of two edges,
 * normalizes the normal, computes d = - (n · v1), and returns the plane equation
 * with components (a, b, c, d).
 *
 * @param triangle The triangle for which to compute the plane equation.
 * @return PlaneEquation The computed plane equation.
 */
inline PlaneEquation computePlaneEquation(const TriangleCoordinates& triangle) {
    // Calculate edge vectors
    const Vertex edge1{ triangle.v2.x - triangle.v1.x, triangle.v2.y - triangle.v1.y, triangle.v2.z - triangle.v1.z };
    const Vertex edge2{ triangle.v3.x - triangle.v1.x, triangle.v3.y - triangle.v1.y, triangle.v3.z - triangle.v1.z };

    // Calculate the normal via cross product
    Vertex normal{
        edge1.y * edge2.z - edge1.z * edge2.y,
        edge1.z * edge2.x - edge1.x * edge2.z,
        edge1.x * edge2.y - edge1.y * edge2.x
    };

    // Normalize the normal
    float norm = std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
    if (norm > 0.0f) {
        normal.x /= norm;
        normal.y /= norm;
        normal.z /= norm;
    }

    // Compute d = - (n · v1)
    const float d = -(normal.x * triangle.v1.x + normal.y * triangle.v1.y + normal.z * triangle.v1.z);

    // Construct the homogeneous vector n̄ = (n.x, n.y, n.z, d)
    return { normal.x, normal.y, normal.z, d };
}

/**
 * @brief Computes the quadric matrix Q for a given triangle.
 *
 * The quadric matrix Q is computed from the homogeneous vector n̄ as:
 * Q = n̄ * n̄^T.
 *
 * @param triangle The triangle for which to compute the quadric matrix.
 * @return Quadric The computed 4x4 quadric matrix.
 */
inline Quadric computeQuadric(const TriangleCoordinates& triangle) {
    PlaneEquation plane = computePlaneEquation(triangle);
    const std::array n_bar = { plane.a, plane.b, plane.c, plane.d };

    Quadric quadric;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            quadric(i, j) = n_bar[i] * n_bar[j];
        }
    }
    return quadric;
}

/**
 * @brief Computes the quadric matrix Q for a plane equation.
 *
 * The quadric matrix Q is computed from the homogeneous vector n̄ as:
 * Q = n̄ * n̄^T.
 *
 * @param plane The plane equation for which to compute the quadric matrix.
 * @return Quadric The computed 4x4 quadric matrix.
 */
inline Quadric computeQuadricPlaneEquation(const PlaneEquation& plane) {
    const std::array n_bar = { plane.a, plane.b, plane.c, plane.d };

    Quadric quadric;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            quadric(i, j) = n_bar[i] * n_bar[j];
        }
    }
    return quadric;
}

/**
 * @brief Evaluates the quadric error for a given point with respect to a quadric.
 *
 * The point is considered in homogeneous coordinates (x, y, z, 1) and the error is given by:
 *   error = p^T * Q * p.
 *
 * @param quadric The quadric.
 * @param point The point (Vertex) to evaluate.
 * @return float The computed error.
 */
inline float evaluateError(const Quadric& quadric, const Vertex& point) {
    const float p[4] = { point.x, point.y, point.z, 1.0f };
    float error = 0.0f;
    for (int i = 0; i < 4; ++i) {
        float sum = 0.0f;
        for (int j = 0; j < 4; ++j) {
            sum += quadric(i, j) * p[j];
        }
        error += p[i] * sum;
    }
    return error;
}

/**
 * @brief Aggregates multiple quadrics into a single quadric by element-wise summation.
 *
 * @param quadrics A vector containing multiple quadrics.
 * @return Quadric The aggregated quadric.
 */
inline Quadric addQuadrics(const std::vector<Quadric>& quadrics) {
    Quadric aggregated;

    // Sum the quadrics element-wise
    for (const auto& Q : quadrics) {
        for (int i = 0; i < 16; ++i) {
            aggregated.data[i] += Q.data[i];
        }
    }
    return aggregated;
}

/**
 * @brief Adds two quadric matrices element-wise.
 *
 * @param quadric The first quadric matrix.
 * @param quadric2 The second quadric matrix.
 * @return Quadric The sum of the two quadric matrices.
 */
inline Quadric addQuadric(Quadric & quadric, const Quadric & quadric2) {

    for (int i = 0; i < 16; ++i) {
        quadric.data[i] += quadric2.data[i];
    }
    
    return quadric;
}

/**
 * @brief Evaluates the total error for a given point with respect to multiple quadric matrices.
 *
 * This function computes the sum of individual errors:
 *     total_error = Σ (p^T * Q_i * p)
 *
 * @param quadrics A vector of 4x4 quadric matrices.
 * @param point The point (Vertex) to evaluate.
 * @return float The total error value.
 */
inline float evaluateTotalError(const std::vector<Quadric>& quadrics, const Vertex& point) {
    float totalError = 0.0f;
    for (const auto& Q : quadrics) {
        totalError += evaluateError(Q, point);
    }
    return totalError;
}


/**
 * @brief Finds the optimal point that minimizes the quadric error.
 * 
 * This function solves the linear system A*x = -b to find the point
 * that minimizes the quadric error. If the system is not solvable,
 * it returns the origin (0, 0, 0).
 *
 * @param quadric 4x4 quadric matrices.
 * @return a Vertex that is the optimal vertex position for a given quadric matrix.
 */
inline Vertex findOptimalVertex(const Quadric& quadric) {
    // Extract the 3x3 upper-left submatrix A
    Eigen::Matrix3f A;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            A(i, j) = quadric(i, j); 
        }
    }
    
    // Extract the right side vector -b 
    Eigen::Vector3f b;
    b(0) = -quadric(0, 3);  
    b(1) = -quadric(1, 3); 
    b(2) = -quadric(2, 3);  
    
    // Try to solve the system A*x = b
    Eigen::Vector3f result;
    
    // Check if matrix is invertible 
    Eigen::FullPivLU<Eigen::Matrix3f> lu(A);
    if (lu.isInvertible()) {
        // Matrix is invertible, solve the system
        result = A.fullPivLu().solve(b);
        return {result(0), result(1), result(2)};
    } else {
        // Use LDLT to solve
        Eigen::LDLT<Eigen::Matrix3f> ldlt(A);
        if (ldlt.info() == Eigen::Success) {
            result = ldlt.solve(b);
            
            // Check if the result is valid
            bool valid = true;
            for (int i = 0; i < 3; ++i) {
                if (std::isnan(result(i))) {
                    valid = false;
                    break;
                }
            }
            
            if (valid) {
                return {result(0), result(1), result(2)};
            }
        }
        
        return {0.0f, 0.0f, 0.0f};
    }

} 

inline bool isOnEdge(const Vertex &point, const Vertex &vertexA, const Vertex &vertexB)
{
    if (point.x < std::min(vertexA.x, vertexB.x) || point.x > std::max(vertexA.x, vertexB.x))
    {
        return false;
    }
    if (point.y < std::min(vertexA.y, vertexB.y) || point.y > std::max(vertexA.y, vertexB.y))
    {
        return false;
    }
    if (point.z < std::min(vertexA.z, vertexB.z) || point.z > std::max(vertexA.z, vertexB.z))
    {
        return false;
    }

    Vertex vecAP = {
        point.x - vertexA.x,
        point.y - vertexA.y,
        point.z - vertexA.z};

    Vertex vecAB = {
        vertexB.x - vertexA.x,
        vertexB.y - vertexA.y,
        vertexB.z - vertexA.z};
    
    float lengthAB = sqrt(vecAB.x*vecAB.x + vecAB.y*vecAB.y + vecAB.z*vecAB.z);
    
    float crossX = vecAP.y * vecAB.z - vecAP.z * vecAB.y;
    float crossY = vecAP.z * vecAB.x - vecAP.x * vecAB.z;
    float crossZ = vecAP.x * vecAB.y - vecAP.y * vecAB.x;

    float crossMagnitude = sqrt(crossX*crossX + crossY*crossY + crossZ*crossZ);

    if (crossMagnitude / lengthAB > 1e-6) {
        return false; 
    }

    return true;
}

inline Vertex findOptimalVertex(const Quadric &quadric, const Vertex &vA, const Vertex &vB)
{
    // 1) on reconstruit la matrice Eigen
    Eigen::Matrix4f Q;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            Q(i, j) = quadric(i, j);

    // Partition
    Eigen::Matrix3f A = Q.topLeftCorner<3, 3>();
    Eigen::Vector3f b = -Q.topRightCorner<3, 1>();

    // 1) tentative QR
    Eigen::Vector3f sol = A.colPivHouseholderQr().solve(b);
    bool good = (A * sol).isApprox(b, 1e-6f);

    if (good)
    {
        // 2) clamp sur [vA,vB]
        Eigen::Vector3f VA{vA.x, vA.y, vA.z},
            VB{vB.x, vB.y, vB.z};
        Eigen::Vector3f d = VB - VA;
        float t = d.dot(sol - VA) / d.squaredNorm();
        t = std::clamp(t, 0.0f, 1.0f);
        Eigen::Vector3f proj = VA + t * d;

        // 3) comparer erreurs
        auto err = [&](const Eigen::Vector3f &v3)
        {
            Eigen::Vector4f vh{v3(0), v3(1), v3(2), 1.f};
            return vh.transpose() * Q * vh;
        };
        float eA = err(VA), eB = err(VB), eP = err(proj);
        if (eA <= eB && eA <= eP)
            return vA;
        else if (eB <= eA && eB <= eP)
            return vB;
        else
            return Vertex{proj(0), proj(1), proj(2)};
    }

    // 4) fallback 4×4 (optionnel) puis endpoints
    Eigen::Matrix4f Qhat = Q;
    Qhat.row(3) = Eigen::Vector4f(0, 0, 0, 1);
    Qhat.col(3) = Eigen::Vector4f(0, 0, 0, 1);
    if (std::abs(Qhat.determinant()) > 1e-6f)
    {
        auto sol4 = Qhat.inverse() * Eigen::Vector4f(0, 0, 0, 1);
        return {sol4(0) / sol4(3), sol4(1) / sol4(3), sol4(2) / sol4(3)};
    }

    // 5) ultime fallback : comparer vA/vB
    float eA = ([&]()
                {
    Eigen::Vector4f vh{vA.x,vA.y,vA.z,1.f};
    return vh.transpose()*Q*vh; })();
        float eB = ([&]()
                    {
    Eigen::Vector4f vh{vB.x,vB.y,vB.z,1.f};
    return vh.transpose()*Q*vh; })();
        return (eA < eB) ? vA : vB;

}

#endif // QUADRIC_CALCULATOR_H
