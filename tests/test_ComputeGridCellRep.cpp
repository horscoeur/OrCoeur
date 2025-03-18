#include "mesh_simplification.h"
#include "../src/mesh_simplification.cpp" // TODO: Include the source file to run the tests on
#include "../src/dereference_passes.cpp"  
#include "merge_sort.h"         
#include "structures.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fstream>
#include <vector>
#include <filesystem>

// Helper function to create a test file with grid plane entries
void createTestPlaneEquationFile(const std::string& filename, 
                               const std::vector<GridPlaneEntry>& entries) {
    std::ofstream file(filename, std::ios::binary);
    REQUIRE(file.is_open());
    
    for (const auto& entry : entries) {
        file.write(reinterpret_cast<const char*>(&entry), sizeof(GridPlaneEntry));
    }
    file.close();
}

// Helper function to read representatives from a file
std::map<int, Vertex> readRepresentativesFile(const std::string& filename) {
    std::map<int, Vertex> result;
    std::ifstream file(filename, std::ios::binary);
    REQUIRE(file.is_open());
    
    int gridIndex;
    Vertex vertex;
    
    while (file.read(reinterpret_cast<char*>(&gridIndex), sizeof(int)) &&
           file.read(reinterpret_cast<char*>(&vertex), sizeof(Vertex))) {
        result[gridIndex] = vertex;
    }
    
    file.close();
    return result;
}

TEST_CASE("computeGridCellRepresentatives error handling", "[GridCellRepresentatives]") {
    SECTION("Non-existent input file") {
        bool result = computeGridCellRepresentatives("non_existent_file.bin", "output.bin");
        REQUIRE_FALSE(result);
    }
    
    // Clean up after test
    std::filesystem::remove("output.bin");
}

TEST_CASE("computeGridCellRepresentatives with single grid cell", "[GridCellRepresentatives]") {
    // Create a test file with a single grid cell containing a plane
    std::vector<GridPlaneEntry> entries;
    
    // Plane equation for z=0 (normal {0,0,1}, distance 0)
    PlaneEquation plane1 = {0.0f, 0.0f, 1.0f, 0.0f};
    entries.push_back({1, plane1}); // Grid index 1
    
    std::string inputFile = "test_single_cell.bin";
    std::string outputFile = "test_single_cell_out.bin";
    
    createTestPlaneEquationFile(inputFile, entries);
    
    // Execute the function
    bool result = computeGridCellRepresentatives(inputFile, outputFile);
    REQUIRE(result);
    
    // Read and verify the result
    auto representatives = readRepresentativesFile(outputFile);
    
    REQUIRE(representatives.size() == 1);
    REQUIRE(representatives.count(1) == 1);
    
    // The optimal vertex for a plane z=0 should have z=0
    REQUIRE(representatives[1].z == Catch::Approx(0.0f).margin(1e-5f));
    
    // Clean up
    std::filesystem::remove(inputFile);
    std::filesystem::remove(outputFile);
}

TEST_CASE("computeGridCellRepresentatives with multiple grid cells", "[GridCellRepresentatives]") {
    std::vector<GridPlaneEntry> entries;
    
    // Cell 1: Plane z=0
    PlaneEquation plane1 = {0.0f, 0.0f, 1.0f, 0.0f};
    entries.push_back({1, plane1});
    
    // Cell 2: Intersection of three planes at (1,2,3)
    PlaneEquation plane2a = {1.0f, 0.0f, 0.0f, -1.0f};  // x=1
    PlaneEquation plane2b = {0.0f, 1.0f, 0.0f, -2.0f};  // y=2
    PlaneEquation plane2c = {0.0f, 0.0f, 1.0f, -3.0f};  // z=3
    
    entries.push_back({2, plane2a});
    entries.push_back({2, plane2b});
    entries.push_back({2, plane2c});
    
    // Cell 3: Non-invertible case with parallel planes
    PlaneEquation plane3a = {0.0f, 0.0f, 1.0f, -5.0f};  // z=5
    PlaneEquation plane3b = {0.0f, 0.0f, 1.0f, -5.0f};  // z=5 (duplicate)
    
    entries.push_back({3, plane3a});
    entries.push_back({3, plane3b});
    
    std::string inputFile = "test_multiple_cells.bin";
    std::string outputFile = "test_multiple_cells_out.bin";
    
    createTestPlaneEquationFile(inputFile, entries);
    
    // Execute the function
    bool result = computeGridCellRepresentatives(inputFile, outputFile);
    REQUIRE(result);
    
    // Read and verify the results
    auto representatives = readRepresentativesFile(outputFile);
    
    REQUIRE(representatives.size() == 3);
    
    // Cell 1: z=0 plane
    REQUIRE(representatives.count(1) == 1);
    REQUIRE(representatives[1].z == Catch::Approx(0.0f).margin(1e-5f));
    
    // Cell 2: Intersection at (1,2,3)
    REQUIRE(representatives.count(2) == 1);
    REQUIRE(representatives[2].x == Catch::Approx(1.0f).margin(1e-5f));
    REQUIRE(representatives[2].y == Catch::Approx(2.0f).margin(1e-5f));
    REQUIRE(representatives[2].z == Catch::Approx(3.0f).margin(1e-5f));
    
    // Cell 3: Non-invertible case, should be at z=5
    REQUIRE(representatives.count(3) == 1);
    REQUIRE(representatives[3].z == Catch::Approx(5.0f).margin(1e-5f));
    
    // Clean up
    std::filesystem::remove(inputFile);
    std::filesystem::remove(outputFile);
}
