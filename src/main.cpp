#include "polyscope/polyscope.h"
#include "ui.h"
#include "imfilebrowser.h"
#include "mesh_conversion.h"
#include "mesh_loader.h"

/**
 * @brief Main function initializing Polyscope and handling UI.
 */
int main(int argc, char **argv) {
    // Initialize Polyscope
    polyscope::init();

    // Apply ImGui style
    polyscope::options::configureImGuiStyleCallback = configureImGuiStyle;

    // Remove maxFPS option (if set, the limit leads to abusively high CPU usage)
    polyscope::options::maxFPS = -1;

    // Register user callback for UI
    polyscope::state::userCallback = [&]() {
        handleFileSelection();
        loadMeshUI();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        conversionUI();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        normalMeshSimplificationUI();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        adaptiveMeshSimplificationUI();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        streamSimplificationPopup();
    };

    // Show Polyscope GUI
    polyscope::show();

    return 0;
}
