#include "quadrics.h"

#include <array>
#include <vector>
#include <cmath>
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

// Utility function to compare two 4x4 matrices with a given tolerance
bool matricesEqual(const std::array<float, 16> &A, const std::array<float, 16> &B, float tol = 1e-5f) {
    for (int i = 0; i < 16; ++i) {
        if (std::fabs(A[i] - B[i]) > tol)
            return false;
    }
    return true;
}

TEST_CASE("ComputeQuadric for non-degenerate triangle in XY plane", "[QuadricCalculator]") {
    // Triangle in the plane z = 0
    constexpr TriangleCoordinates triangle = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    const auto Q = computeQuadric(triangle);
    // For this triangle, the calculated normal is (0, 0, 1) and d = 0,
    // so n̄ = (0, 0, 1, 0) and Q = n̄ * n̄^T.
    // The expected Q matrix has only one non-zero coefficient Q(2, 2) = 1.
    constexpr std::array expected = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    REQUIRE(matricesEqual(Q.data, expected));
}

TEST_CASE("EvaluateError for point on the plane", "[QuadricCalculator]") {
    // For the same triangle (plane z=0), any point with coordinates (x, y, 0) should have zero error.
    constexpr TriangleCoordinates triangle = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    const auto Q = computeQuadric(triangle);
    constexpr Vertex p_on_plane = {0.3f, 0.3f, 0.0f};
    const float error = evaluateError(Q, p_on_plane);
    REQUIRE(error == Catch::Approx(0.0f).epsilon(1e-5));
}

TEST_CASE("EvaluateError for point off the plane", "[QuadricCalculator]") {
    // For the same triangle, a point with a z coordinate != 0 should produce an error equal to z^2.
    constexpr TriangleCoordinates triangle = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    const auto Q = computeQuadric(triangle);
    constexpr Vertex p_off_plane = {0.3f, 0.3f, 0.5f}; // z = 0.5
    const float error = evaluateError(Q, p_off_plane);
    // p^T * Q * p = (0.5)^2 = 0.25
    REQUIRE(error == Catch::Approx(0.25f).epsilon(1e-5));
}

TEST_CASE("AddQuadrics sums matrices correctly", "[QuadricCalculator]") {
    // Using two triangles defining two parallel planes (z = 0 and z = 1)
    constexpr TriangleCoordinates triangle1 = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    constexpr TriangleCoordinates triangle2 = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}
    };

    const auto Q1 = computeQuadric(triangle1);
    const auto Q2 = computeQuadric(triangle2);
    const std::vector quadrics = {Q1, Q2};

    const auto aggregated = addQuadrics(quadrics);

    // For triangle1, Q1 has only one non-zero coefficient Q1[2][2] = 1.
    // For triangle2, with a normal (0, 0, 1) and d = -1, n̄ = (0, 0, 1, -1),
    // Q2 has: Q2(2, 2) = 1, Q2(3, 3) = 1, Q2(2, 3) = Q2(3, 2) = -1.
    // The expected sum is:
    constexpr std::array expected = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 2.0f, -1.0f,
        0.0f, 0.0f, -1.0f, 1.0f
    };
    REQUIRE(matricesEqual(aggregated.data, expected));
}

TEST_CASE("EvaluateTotalError sums errors correctly", "[QuadricCalculator]") {
    // Using the same two triangles as in the previous test.
    constexpr TriangleCoordinates triangle1 = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };
    constexpr TriangleCoordinates triangle2 = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f}
    };
    const auto Q1 = computeQuadric(triangle1);
    const auto Q2 = computeQuadric(triangle2);
    const std::vector quadrics = {Q1, Q2};

    // For a point p = (0.5, 0.5, 0.5)
    // On the plane z=0 (triangle1): error = (0.5)^2 = 0.25.
    // On the plane z=1 (triangle2): error = (0.5)^2 - 2*0.5*1 + 1 = 0.25.
    // Expected total error: 0.25 + 0.25 = 0.5.
    constexpr Vertex p = {0.5f, 0.5f, 0.5f};
    const float totalError = evaluateTotalError(quadrics, p);
    REQUIRE(totalError == Catch::Approx(0.5f).epsilon(1e-5));
}

TEST_CASE("ComputeQuadric for degenerate (collinear) triangle", "[QuadricCalculator]") {
    // Degenerate triangle: the points are collinear (example: (0,0,0), (1,1,1), (2,2,2)).
    constexpr TriangleCoordinates degenerateTriangle = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 1.0f},
        {2.0f, 2.0f, 2.0f}
    };
    const auto Q = computeQuadric(degenerateTriangle);
    // In this case, the normal vector will be zero and the Q matrix should be a zero matrix.
    constexpr std::array expected = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    REQUIRE(matricesEqual(Q.data, expected));
}

TEST_CASE("EvaluateError returns zero for zero quadric", "[QuadricCalculator]") {
    // A special case: for a zero quadric matrix, the error should be zero for any point.
    constexpr std::array zeroQuadric = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f
    };
    constexpr Vertex p = {10.0f, -5.0f, 3.0f};
    const float error = evaluateError(Quadric(zeroQuadric), p);
    REQUIRE(error == Catch::Approx(0.0f).epsilon(1e-5));
}
