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

# Add the executable
add_executable(OrCoeur ${SOURCES} ${HEADERS})

# Add DEBUG flag to enable debug mode
target_compile_definitions(OrCoeur PRIVATE -DDEBUG)

# Link Polyscope to the executable
target_link_libraries(OrCoeur polyscope)