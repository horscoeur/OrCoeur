cmake_minimum_required(VERSION 3.20)
project(OrCoeur)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile with -O3 optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Use FetchContent for Eigen (moved outside of test conditional)
include(FetchContent)
FetchContent_Declare(
    eigen
    GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
    GIT_TAG 3.4.0
)
set(BUILD_TESTING OFF)
set(EIGEN_BUILD_TESTING OFF)
set(EIGEN_MPL2_ONLY ON)
set(EIGEN_BUILD_PKGCONFIG OFF)
set(EIGEN_BUILD_DOC OFF)
FetchContent_MakeAvailable(eigen)

# Include Polyscope
add_subdirectory(external/polyscope)
include_directories(external/polyscope/include)
include_directories(external/imgui-filebrowser)
include_directories(include)
include_directories(${CMAKE_SOURCE_DIR}/external/polyscope/deps/imgui/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/polyscope/deps/imgui/imgui)

# Add imgui source files
file(GLOB IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/external/polyscope/deps/imgui/imgui/*.cpp
)

# Automatic retrieval of header files
file(GLOB_RECURSE HEADERS include/*.h)

# Liste tous les fichiers sources sauf main.cpp
file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp)
list(REMOVE_ITEM PROJECT_SOURCES ${CMAKE_SOURCE_DIR}/src/main.cpp)

# Crée une bibliothèque statique avec le code commun
add_library(orcoeur_lib STATIC ${PROJECT_SOURCES})

target_link_libraries(orcoeur_lib PRIVATE polyscope Eigen3::Eigen)

# Exécutable principal
add_executable(OrCoeur src/main.cpp ${HEADERS} ${IMGUI_SOURCES})
target_link_libraries(OrCoeur PRIVATE orcoeur_lib)

# Tests
option(ORCOEUR_BUILD_TESTS "Build unit tests" ON)
if(ORCOEUR_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(catch2)
    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)
    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests PRIVATE orcoeur_lib Catch2::Catch2WithMain Eigen3::Eigen)
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif()