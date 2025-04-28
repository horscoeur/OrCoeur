#include <catch2/catch_test_macros.hpp>
#include "../include/stream_simplification.h"

static StreamMeshData makeSimpleFanMesh()
{
    // Same setup as the first test: two coplanar triangles sharing edge (0,2)
    Vertex v0{0.0f,0.0f,0.0f},
           v1{1.0f,0.0f,0.0f},
           v2{1.0f,1.0f,0.0f},
           v3{0.0f,1.0f,0.0f};
    TriangleCoordinates tri0{v0, v1, v2};
    TriangleCoordinates tri1{v0, v2, v3};

    StreamMeshData mesh;
    mesh.actual_unique_id = 4;
    mesh.vertexMap = {{v0,0},{v1,1},{v2,2},{v3,3}};
    mesh.inCoreTriangleBuffer = {{0,tri0},{1,tri1}};
    mesh.triangleList = {
        {0,{0,1}},
        {2,{0,1}},
        {1,{0}},
        {3,{1}}
    };
    return mesh;
}

TEST_CASE("isCollapseValid returns false when vertices share no triangles",
          "[isCollapseValid]")
{
    StreamMeshData mesh;
    mesh.actual_unique_id = 2;
    // two isolated vertices, no triangles at all
    mesh.vertexMap = {{Vertex{0,0,0},0},{Vertex{1,1,1},1}};
    mesh.inCoreTriangleBuffer.clear();
    mesh.triangleList.clear();
    Vertex p{0.5f,0.5f,0.5f};
    // no shared triangles => invalid
    REQUIRE(isCollapseValid(0, 1, p, mesh) == false);
}

TEST_CASE("isCollapseValid returns true when collapse preserves orientation",
          "[isCollapseValid][preserve]")
{
    auto mesh = makeSimpleFanMesh();
    // collapse to a point above the XY plane => normals stay positive
    Vertex above{0.5f, 0.5f, 1.0f};
    REQUIRE(isCollapseValid(0, 2, above, mesh) == true);
}
