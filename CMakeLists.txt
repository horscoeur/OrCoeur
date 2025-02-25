cmake_minimum_required(VERSION 3.20)
project(OrCoeur)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compile with -O3 optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

# Include Polyscope
add_subdirectory(external/polyscope)
include_directories(external/polyscope/include)
include_directories(external/imgui-filebrowser)
include_directories(include)

# Automatic retrieval of source files
file(GLOB_RECURSE SOURCES src/*.cpp)

# Automatic retrieval of header files
file(GLOB_RECURSE HEADERS include/*.h)

# Add the main executable
add_executable(OrCoeur ${SOURCES} ${HEADERS})

# Add DEBUG flag to enable debug mode
target_compile_definitions(OrCoeur PRIVATE -DDEBUG)

# Link Polyscope to the executable
target_link_libraries(OrCoeur polyscope)

# Option to activate the tests (disabled by default)
option(ORCOEUR_BUILD_TESTS "Build unit tests" ON)
if(ORCOEUR_BUILD_TESTS)
    enable_testing()

    # Use FetchContent to automatically download and build Catch2
    include(FetchContent)
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
    target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

    # Automatic test discovery with Catch2
    include(CTest)
    include(Catch)
    catch_discover_tests(tests)
endif()
