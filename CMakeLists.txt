cmake_minimum_required(VERSION 3.20)
project(OrCoeur)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Polyscope
add_subdirectory(external/polyscope)
include_directories(external/polyscope/include)

# Add the executable
add_executable(OrCoeur main.cpp)

# Link Polyscope to the executable
target_link_libraries(OrCoeur polyscope)