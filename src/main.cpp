#include "polyscope/polyscope.h"
#include "ui.h"
#include "imfilebrowser.h"

/**
 * @brief Main function initializing Polyscope and handling UI.
 */
int main(int argc, char** argv) {
    // Initialize Polyscope
    polyscope::init();

    // File selection dialog
    char filename[2048] = "";
    ImGui::FileBrowser fileDialog;
    fileDialog.SetTitle("Open a mesh file");
    fileDialog.SetTypeFilters({ ".obj", ".ply", ".stl" });

    // Apply ImGui style
    polyscope::options::configureImGuiStyleCallback = configureImGuiStyle;

    // Register user callback for UI
    polyscope::state::userCallback = [&]() {
        handleFileSelection(filename, fileDialog);
    };

    // Show Polyscope GUI
    polyscope::show();

    return 0;
}
