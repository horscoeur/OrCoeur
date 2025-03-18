#include "mesh_simplification.h"
#include "structures.h"
#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <fstream>
#include <vector>
#include <map>
#include <filesystem>

// Fonctions d'aide pour créer des fichiers de test avec un namespace pour éviter les conflits
namespace dereference_test {

    // Helper function pour créer un fichier avec des représentants
    void createRepresentativesFile(const std::string& filename, 
                                const std::map<int, Vertex>& representatives) {
        std::ofstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        for (const auto& [index, vertex] : representatives) {
            file.write(reinterpret_cast<const char*>(&index), sizeof(int));
            file.write(reinterpret_cast<const char*>(&vertex), sizeof(Vertex));
        }
        file.close();
    }
    
    // Helper function pour créer un fichier de cluster (indices de triangles)
    void createClusterFile(const std::string& filename, 
                        const std::vector<TriangleIndices>& triangles) {
        std::ofstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        file.write(reinterpret_cast<const char*>(triangles.data()), 
                 triangles.size() * sizeof(TriangleIndices));
        file.close();
    }
    
    // Lire les triangles après le pass 1
    std::vector<Triangle_Pass1> readPass1File(const std::string& filename) {
        std::vector<Triangle_Pass1> result;
        std::ifstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        Triangle_Pass1 triangle;
        while (file.read(reinterpret_cast<char*>(&triangle), sizeof(Triangle_Pass1))) {
            result.push_back(triangle);
        }
        
        file.close();
        return result;
    }
    
    // Lire les triangles après le pass 2
    std::vector<Triangle_Pass2> readPass2File(const std::string& filename) {
        std::vector<Triangle_Pass2> result;
        std::ifstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        Triangle_Pass2 triangle;
        while (file.read(reinterpret_cast<char*>(&triangle), sizeof(Triangle_Pass2))) {
            result.push_back(triangle);
        }
        
        file.close();
        return result;
    }
    
    // Lire les triangles finaux
    std::vector<TriangleCoordinates> readFinalMesh(const std::string& filename) {
        std::vector<TriangleCoordinates> result;
        std::ifstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        TriangleCoordinates triangle;
        while (file.read(reinterpret_cast<char*>(&triangle), sizeof(TriangleCoordinates))) {
            result.push_back(triangle);
        }
        
        file.close();
        return result;
    }
}

TEST_CASE("dereferenceClusterPass1 basic functionality", "[DereferenceCluster]") {
    std::map<int, Vertex> representatives = {
        {1, {1.0f, 2.0f, 3.0f}},
        {2, {4.0f, 5.0f, 6.0f}},
        {3, {7.0f, 8.0f, 9.0f}}
    };
    
    std::vector<TriangleIndices> triangles = {
        {1, 2, 3},
        {2, 3, 1},
        {3, 1, 2}
    };
    
    std::string repsFile = "test_reps.bin";
    std::string clusterFile = "test_cluster.bin";
    std::string outputFile = "test_pass1.bin";
    
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    REQUIRE(dereferenceClusterPass1(repsFile, clusterFile, outputFile));
    
    // verify that the output file contains the expected data
    auto results = dereference_test::readPass1File(outputFile);
    REQUIRE(results.size() == 3);
    
    // verify that the triangles are correctly dereferenced
    REQUIRE(results[0].v1.x == Catch::Approx(1.0f));
    REQUIRE(results[0].v1.y == Catch::Approx(2.0f));
    REQUIRE(results[0].v1.z == Catch::Approx(3.0f));
    REQUIRE(results[0].v2 == 2);
    REQUIRE(results[0].v3 == 3);
    
    REQUIRE(results[1].v1.x == Catch::Approx(4.0f));
    REQUIRE(results[1].v1.y == Catch::Approx(5.0f));
    REQUIRE(results[1].v1.z == Catch::Approx(6.0f));
    REQUIRE(results[1].v2 == 3);
    REQUIRE(results[1].v3 == 1);
    
    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(outputFile);
}

TEST_CASE("dereferenceClusterPass1 with missing indices", "[DereferenceCluster]") {
    // test with a cluster file that contains indices that are not in the representatives file
    std::map<int, Vertex> representatives = {
        {1, {1.0f, 2.0f, 3.0f}},
        {3, {7.0f, 8.0f, 9.0f}}
    };
    
    std::vector<TriangleIndices> triangles = {
        {1, 3, 2},  
        {2, 1, 3},  // "2" for v1 doest not exist
        {3, 2, 1}   
    };
    
    std::string repsFile = "test_missing_reps.bin";
    std::string clusterFile = "test_missing_cluster.bin";
    std::string outputFile = "test_missing_pass1.bin";
    
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    REQUIRE(dereferenceClusterPass1(repsFile, clusterFile, outputFile));
    
    // only 2 out of 3 triangles should have a valid v1
    auto results = dereference_test::readPass1File(outputFile);
    REQUIRE(results.size() == 2);  // if we have only 2 triangles it succeeded
    
    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(outputFile);
}

TEST_CASE("Full dereference pipeline", "[DereferenceCluster]") {
    // test with all steps of the dereference pipeline
    std::map<int, Vertex> representatives = {
        {1, {1.0f, 2.0f, 3.0f}},
        {2, {4.0f, 5.0f, 6.0f}},
        {3, {7.0f, 8.0f, 9.0f}}
    };
    
    std::vector<TriangleIndices> triangles = {
        {1, 2, 3},
        {2, 3, 1}
    };
    
    std::string repsFile = "test_full_reps.bin";
    std::string clusterFile = "test_full_cluster.bin";
    std::string finalFile = "test_full_mesh.bin";
    
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    int nbFaces = generateSimplifiedMeshBin(repsFile, clusterFile, finalFile);
    
    REQUIRE(nbFaces == 2);
    auto results = dereference_test::readFinalMesh(finalFile);
    REQUIRE(results.size() == 2);
    
    // check if the triangles are correctly dereferenced regardless of the order
    bool found_triangle1 = false;
    bool found_triangle2 = false;
    
    for (const auto& tri : results) {
        if (Catch::Approx(tri.v1.x) == 1.0f && 
            Catch::Approx(tri.v2.x) == 4.0f && 
            Catch::Approx(tri.v3.x) == 7.0f) {
            found_triangle1 = true;
        }
        else if (Catch::Approx(tri.v1.x) == 4.0f && 
                Catch::Approx(tri.v2.x) == 7.0f && 
                Catch::Approx(tri.v3.x) == 1.0f) {
            found_triangle2 = true;
        }
    }
    
    REQUIRE(found_triangle1);
    REQUIRE(found_triangle2);

    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(finalFile);
}

TEST_CASE("Test repositioning with seekg", "[DereferenceCluster]") {
    std::map<int, Vertex> representatives = {
        {1, {1.0f, 2.0f, 3.0f}},
        {5, {5.0f, 5.0f, 5.0f}},  
        {10, {10.0f, 10.0f, 10.0f}}
    };
    
    std::vector<TriangleIndices> triangles = {
        {1, 2, 3},
        {5, 1, 2},   
        {3, 4, 5},
        {10, 5, 1}, 
    };
    
    std::string repsFile = "test_seekg_reps.bin";
    std::string clusterFile = "test_seekg_cluster.bin";
    std::string outputFile = "test_seekg_pass1.bin";
    
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    REQUIRE(dereferenceClusterPass1(repsFile, clusterFile, outputFile));
    
    // we should have 3 out of 4 triangles with valid v1
    auto results = dereference_test::readPass1File(outputFile);
    REQUIRE(results.size() == 3); 
    
    // 3rd triangle should have v1 = {10.0, 10.0, 10.0}
    REQUIRE(results[2].v1.x == Catch::Approx(10.0f));
    
    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(outputFile);
}


TEST_CASE("Dereference with large number of triangles", "[DereferenceCluster]") {
    const int nbOfRepresentatives = 1000;
    const int nbOfTriangles = 10000;
    
    std::map<int, Vertex> representatives;
    for (int i = 0; i < nbOfRepresentatives; i++) {
        representatives[i] = {
            static_cast<float>(i) * 0.1f,
            static_cast<float>(i) * 0.2f,
            static_cast<float>(i) * 0.3f
        };
    }
    
    std::vector<TriangleIndices> triangles;
    triangles.reserve(nbOfTriangles);
    
    for (int i = 0; i < nbOfTriangles; i++) {
        int v1 = i % nbOfRepresentatives;
        int v2 = (i + 300) % nbOfRepresentatives;
        int v3 = (i + 700) % nbOfRepresentatives;
        
        triangles.push_back({v1, v2, v3});
    }
    
    std::string repsFile = "test_large_reps.bin";
    std::string clusterFile = "test_large_cluster.bin";
    std::string finalFile = "test_large_mesh.bin";
    

    INFO("Creating test with " << nbOfTriangles << " triangles and " << nbOfRepresentatives << " representatives");
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    INFO("Starting pass 1");
    auto start = std::chrono::high_resolution_clock::now();
    REQUIRE(dereferenceClusterPass1(repsFile, clusterFile, "test_large_pass1.bin"));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    INFO("Pass 1 done in " << duration << " ms");
    
    auto results_pass1 = dereference_test::readPass1File("test_large_pass1.bin");
    REQUIRE(results_pass1.size() == nbOfTriangles);  // all triangles should be processed
    
    //check that all triangles have been correctly dereferenced
    std::set<int> found_indices;
    
    for (const auto& tri : results_pass1) {
        float normalized_x = tri.v1.x / 0.1f; 
        int closest_index = static_cast<int>(std::round(normalized_x));
        
        // check that the index is within the expected range
        REQUIRE(closest_index >= 0);
        REQUIRE(closest_index < nbOfRepresentatives);
        
        // check that the vertex is correct
        REQUIRE(tri.v1.y == Catch::Approx(closest_index * 0.2f));
        REQUIRE(tri.v1.z == Catch::Approx(closest_index * 0.3f));
        
        found_indices.insert(closest_index);
    }
    
    // check that all representative indices have been used
    REQUIRE(found_indices.size() == nbOfRepresentatives);
    

    INFO("Starting full pipeline");
    start = std::chrono::high_resolution_clock::now();
    int nbFaces = generateSimplifiedMeshBin(repsFile, clusterFile, finalFile);
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    INFO("Full pipeline done in " << duration << " ms");
    

    REQUIRE(nbFaces == nbOfTriangles);
    
    auto final_triangles = dereference_test::readFinalMesh(finalFile);
    REQUIRE(final_triangles.size() == nbOfTriangles);
    
    //check if we can find the triangle number 42
    int sample_idx = 42;
    int v1_idx = sample_idx % nbOfRepresentatives;
    int v2_idx = (sample_idx + 300) % nbOfRepresentatives;
    int v3_idx = (sample_idx + 700) % nbOfRepresentatives;
    
    bool found_sample = false;
    for (const auto& tri : final_triangles) {
        if (Catch::Approx(tri.v1.x) == representatives[v1_idx].x &&
            Catch::Approx(tri.v2.x) == representatives[v2_idx].x &&
            Catch::Approx(tri.v3.x) == representatives[v3_idx].x) {
            found_sample = true;
            break;
        }
    }
    REQUIRE(found_sample);
    
    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(finalFile);
    std::filesystem::remove("test_large_pass1.bin");
    
    INFO("Test passed");
}


TEST_CASE("Dereference with buffer limit exceeded", "[DereferenceCluster]") {
    // Définir un nombre de triangles qui dépasse légèrement la taille du buffer
    const int BUFFER_SIZE = 1024;
    const int nbOfRepresentatives = 200;
    const int nbOfTriangles = BUFFER_SIZE + 100;  // 1124 triangles
    
    std::map<int, Vertex> representatives;
    for (int i = 0; i < nbOfRepresentatives; i++) {
        representatives[i] = {
            static_cast<float>(i) * 0.1f,
            static_cast<float>(i) * 0.2f,
            static_cast<float>(i) * 0.3f
        };
    }
    
    std::vector<TriangleIndices> triangles;
    triangles.reserve(nbOfTriangles);
    
    for (int i = 0; i < nbOfTriangles; i++) {
        int v1 = i % nbOfRepresentatives;
        int v2 = (i + 300) % nbOfRepresentatives;
        int v3 = (i + 700) % nbOfRepresentatives;
        
        triangles.push_back({v1, v2, v3});
    }
    
    std::string repsFile = "test_large_reps.bin";
    std::string clusterFile = "test_large_cluster.bin";
    std::string finalFile = "test_large_mesh.bin";
    

    INFO("Creating test with " << nbOfTriangles << " triangles and " << nbOfRepresentatives << " representatives");
    dereference_test::createRepresentativesFile(repsFile, representatives);
    dereference_test::createClusterFile(clusterFile, triangles);
    
    INFO("Starting pass 1");
    auto start = std::chrono::high_resolution_clock::now();
    REQUIRE(dereferenceClusterPass1(repsFile, clusterFile, "test_large_pass1.bin"));
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    INFO("Pass 1 done in " << duration << " ms");
    
    auto results_pass1 = dereference_test::readPass1File("test_large_pass1.bin");
    REQUIRE(results_pass1.size() == nbOfTriangles);  // all triangles should be processed
    
    //check that all triangles have been correctly dereferenced
    std::set<int> found_indices;
    
    for (const auto& tri : results_pass1) {
        float normalized_x = tri.v1.x / 0.1f; 
        int closest_index = static_cast<int>(std::round(normalized_x));
        
        // check that the index is within the expected range
        REQUIRE(closest_index >= 0);
        REQUIRE(closest_index < nbOfRepresentatives);
        
        // check that the vertex is correct
        REQUIRE(tri.v1.y == Catch::Approx(closest_index * 0.2f));
        REQUIRE(tri.v1.z == Catch::Approx(closest_index * 0.3f));
        
        found_indices.insert(closest_index);
    }
    
    // check that all representative indices have been used
    REQUIRE(found_indices.size() == nbOfRepresentatives);
    

    INFO("Starting full pipeline");
    start = std::chrono::high_resolution_clock::now();
    int nbFaces = generateSimplifiedMeshBin(repsFile, clusterFile, finalFile);
    end = std::chrono::high_resolution_clock::now();
    
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    INFO("Full pipeline done in " << duration << " ms");
    

    REQUIRE(nbFaces == nbOfTriangles);
    
    auto final_triangles = dereference_test::readFinalMesh(finalFile);
    REQUIRE(final_triangles.size() == nbOfTriangles);
    
    //check if we can find the triangle number 42
    int sample_idx = 42;
    int v1_idx = sample_idx % nbOfRepresentatives;
    int v2_idx = (sample_idx + 300) % nbOfRepresentatives;
    int v3_idx = (sample_idx + 700) % nbOfRepresentatives;
    
    bool found_sample = false;
    for (const auto& tri : final_triangles) {
        if (Catch::Approx(tri.v1.x) == representatives[v1_idx].x &&
            Catch::Approx(tri.v2.x) == representatives[v2_idx].x &&
            Catch::Approx(tri.v3.x) == representatives[v3_idx].x) {
            found_sample = true;
            break;
        }
    }
    REQUIRE(found_sample);
    
    std::filesystem::remove(repsFile);
    std::filesystem::remove(clusterFile);
    std::filesystem::remove(finalFile);
    std::filesystem::remove("test_large_pass1.bin");
    
    INFO("Test passed");
}