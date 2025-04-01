#include "polyscope/polyscope.h"
#include "ui.h"
#include "imfilebrowser.h"
#include "mesh_conversion.h"

/**
 * @brief Main function initializing Polyscope and handling UI.
 */
int main(int argc, char **argv) {
    // Initialize Polyscope
    polyscope::init();

    // File selection dialog
    char filename[2048] = "";
    int resolution = 5;
    ImGui::FileBrowser fileDialog;
    fileDialog.SetTitle("Open a mesh file");
    fileDialog.SetTypeFilters({".obj", ".ply"});
    bool simplfyTheMeshOutOfCore = false;
    // Apply ImGui style
    polyscope::options::configureImGuiStyleCallback = configureImGuiStyle;

    // Remove maxFPS option (if set, the limit leads to abusively high CPU usage)
    polyscope::options::maxFPS = -1;

    // Register user callback for UI
    polyscope::state::userCallback = [&]() {
        handleFileSelection(filename, fileDialog, simplfyTheMeshOutOfCore);
        conversionInfoPopup(filename, fileDialog);
        simplificationOutOfCoreInfoPopup(filename, resolution);
        simplificationInfoPopup(filename, resolution);
    };

    // Show Polyscope GUI
    polyscope::show();

    return 0;
}
