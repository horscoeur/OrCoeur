#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "imfilebrowser.h"
#include "polyscope/point_cloud.h"

/**
 * @brief Configures the ImGui color scheme and style.
 */
void configureImGuiStyle();

/**
 * @brief Handles the file selection dialog and mesh loading.
 * @param filename Buffer to store the selected file path.
 * @param fileDialog ImGui file browser instance.
 * @param displayedPoints Vector containing the displayed point clouds. (adaptative mesh simplification)
 */
void handleFileSelection(char* filename, ImGui::FileBrowser& fileDialog, std::vector<polyscope::PointCloud*> &displayedPoints);

/**
 * @brief Handles the mesh adaptative simplification.
 * @param filename Buffer to store the selected file path.
 * @param fileDialog ImGui file browser instance.
 * @param displayedPoints Vector containing the displayed point clouds.
 */
void adaptativeMeshSimplification (char* filename, ImGui::FileBrowser& fileDialog, std::vector<polyscope::PointCloud*> &displayedPoints);

/**
 * @brief Displays the main conversion popup window.
 * @param filename The selected file path.
 * @param fileDialog ImGui file browser instance.
 */
void conversionInfoPopup(char* filename, ImGui::FileBrowser& fileDialog);


/**
 * @brief Displays the mesh cutting popup window.
 * @param filename The selected file path.
 * @param resolution The resolution of the cutting.
 */
void cuttingInfoPopup(char* filename, int &resolution);


/**
 * @brief Loads a mesh from the given file path.
 * @param filename The path to the mesh file.
 * @param extension The file extension.
 */
void loadMesh(const char* filename, const std::string& extension);

#endif // UI_H