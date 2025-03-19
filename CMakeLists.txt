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

# Automatic retrieval of source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Print the list of source files for debugging
message(STATUS "Source files: ${SOURCES}")

# Automatic retrieval of header files
file(GLOB_RECURSE HEADERS include/*.h)

# Add the main executable
add_executable(OrCoeur ${SOURCES} ${HEADERS} ${IMGUI_SOURCES})

# Add DEBUG flag to enable debug mode
target_compile_definitions(OrCoeur PRIVATE -DDEBUG)

# Link Polyscope and Eigen to the executable (using consistent PRIVATE keyword)
target_link_libraries(OrCoeur PRIVATE polyscope Eigen3::Eigen)

# Option to activate the tests (disabled by default)
option(ORCOEUR_BUILD_TESTS "Build unit tests" OFF)
if(ORCOEUR_BUILD_TESTS)
    enable_testing()

    # Use FetchContent to automatically download and build Catch2
    FetchContent_Declare(
            catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG v3.4.0
    )
    FetchContent_MakeAvailable(catch2)

    # Automatic retrieval of test source files
    file(GLOB_RECURSE TEST_SOURCES tests/*.cpp)

    # Add the test executable
    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain Eigen3::Eigen)

    # Automatic test discovery with Catch2
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif()