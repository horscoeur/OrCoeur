#ifndef QUADRIC_CALCULATOR_H
#define QUADRIC_CALCULATOR_H

#include "structures.h"
#include <array>
#include <vector>
#include <cmath>

/**
 * @brief Represents a plane equation in homogeneous coordinates.
 *
 * The plane is defined by the equation:
 *     a*x + b*y + c*z + d = 0,
 * where (a, b, c) is a normalized normal vector and d is the offset.
 */
struct PlaneEquation {
    float a, b, c, d;
};

/**
 * @brief Represents a quadric as a 4x4 matrix stored in a flat array.
 */
struct Quadric {
    std::array<float, 16> data{};

    constexpr float& operator()(const int i, const int j) { return data[i * 4 + j]; }
    constexpr const float& operator()(const int i, const int j) const { return data[i * 4 + j]; }
};

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

#endif // QUADRIC_CALCULATOR_H
