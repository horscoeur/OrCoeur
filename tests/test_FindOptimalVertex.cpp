#include "quadrics.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

TEST_CASE("findOptimalVertex with plane z=0", "[OptimalVertex]") {
    // Plane z=0
    PlaneEquation plane = {0.0f, 0.0f, 1.0f, 0.0f};
    Quadric q = computeQuadricPlaneEquation(plane);
    
    Vertex optimal = findOptimalVertex(q);
    
    // the optimal point should be on the plane z=0
    REQUIRE(optimal.z == Catch::Approx(0.0f).margin(1e-5f));
}

TEST_CASE("findOptimalVertex with three intersecting planes", "[OptimalVertex]") {
    // 3 planes intersecting at (1, 2, 3)
    PlaneEquation plane1 = {1.0f, 0.0f, 0.0f, -1.0f};  // x=1
    PlaneEquation plane2 = {0.0f, 1.0f, 0.0f, -2.0f};  // y=2
    PlaneEquation plane3 = {0.0f, 0.0f, 1.0f, -3.0f};  // z=3
    
    Quadric q1 = computeQuadricPlaneEquation(plane1);
    Quadric q2 = computeQuadricPlaneEquation(plane2);
    Quadric q3 = computeQuadricPlaneEquation(plane3);
    
    Quadric combined = q1;
    addQuadric(combined, q2);
    addQuadric(combined, q3);
    
    Vertex optimal = findOptimalVertex(combined);
    
    REQUIRE(optimal.x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(optimal.y == Catch::Approx(2.0f).margin(1e-5f));
    REQUIRE(optimal.z == Catch::Approx(3.0f).margin(1e-5f));
}

TEST_CASE("findOptimalVertex with singular matrix", "[OptimalVertex]") {
    // two parallel planes (z=2)
    PlaneEquation plane1 = {0.0f, 0.0f, 1.0f, -2.0f};  
    PlaneEquation plane2 = {0.0f, 0.0f, 1.0f, -2.0f};  
    
    Quadric q1 = computeQuadricPlaneEquation(plane1);
    Quadric q2 = computeQuadricPlaneEquation(plane2);
    
    Quadric combined = q1;
    addQuadric(combined, q2);
    
    Vertex optimal = findOptimalVertex(combined);
    
    // the optimal point should be on the plane z=2
    REQUIRE(optimal.z == Catch::Approx(2.0f).margin(1e-5f));
}

TEST_CASE("findOptimalVertex minimizes error", "[OptimalVertex]") {
    // quadric based on two triangles
    TriangleCoordinates tri1 = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
    };
    TriangleCoordinates tri2 = {
        {0.0f, 0.0f, 0.1f}, {1.0f, 0.0f, 0.1f}, {0.0f, 1.0f, 0.1f}
    };
    
    Quadric q1 = computeQuadric(tri1);
    Quadric q2 = computeQuadric(tri2);
    
    Quadric combined = q1;
    addQuadric(combined, q2);
    
    Vertex optimal = findOptimalVertex(combined);
    float optimalError = evaluateError(combined, optimal);
    
    // check that the error is minimized at the optimal point
    Vertex test1 = {optimal.x + 0.1f, optimal.y, optimal.z};
    Vertex test2 = {optimal.x, optimal.y + 0.1f, optimal.z};
    Vertex test3 = {optimal.x, optimal.y, optimal.z + 0.1f};
    
    REQUIRE(evaluateError(combined, test1) >= optimalError);
    REQUIRE(evaluateError(combined, test2) >= optimalError);
    REQUIRE(evaluateError(combined, test3) >= optimalError);
}

TEST_CASE("findOptimalVertex with non invertible matrix, uses svd", "[OptimalVertex]") {
    // non manifold case where the quadric matrix is not invertible
    TriangleCoordinates tri1 = {
        {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}
    };
    TriangleCoordinates tri2 = {
        {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}
    };
    
    Quadric q1 = computeQuadric(tri1);
    Quadric q2 = computeQuadric(tri2);
    
    Quadric combined = q1;
    addQuadric(combined, q2);
    
    Vertex optimal = findOptimalVertex(combined);
    
    // optimal vertex should be in the plane z=0
    REQUIRE(optimal.z == Catch::Approx(0.0f).margin(1e-5f));

    // optimal vertex should be in the range [0, 1]
    REQUIRE(optimal.x >= 0.0f - 1e-5f);
    REQUIRE(optimal.x <= 1.0f + 1e-5f);
    REQUIRE(optimal.y >= 0.0f - 1e-5f);
    REQUIRE(optimal.y <= 1.0f + 1e-5f);
}