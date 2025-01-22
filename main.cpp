#include "polyscope/polyscope.h"
#include "polyscope/surface_mesh.h"

int main(int argc, char** argv) {
    // Initialise Polyscope
    polyscope::init();

    // Create a simple tetrahedron
    std::vector<std::array<double, 3>> vertices = {
        {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}
    };
    std::vector<std::array<int, 3>> faces = {
        {0, 1, 2}, {0, 1, 3}, {0, 2, 3}, {1, 2, 3}
    };

    // Register the mesh with Polyscope
    polyscope::registerSurfaceMesh("Simple Tetrahedron", vertices, faces);

    // Show the Polyscope GUI
    polyscope::show();

    return 0;
}