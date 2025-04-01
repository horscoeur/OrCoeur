#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "imfilebrowser.h"

/**
 * @brief Configures the ImGui color scheme and style.
 */
void configureImGuiStyle();

/**
 * @brief Handles the file selection dialog and mesh loading.
 * @param filename Buffer to store the selected file path.
 * @param fileDialog ImGui file browser instance.
 * @param simplfyTheMeshOutOfCore Boolean to check if the mesh should be simplified out of core.
 */
void handleFileSelection(char* filename, ImGui::FileBrowser& fileDialog,bool &simplfyTheMeshOutOfCore);

/**
 * @brief Displays the main conversion popup window.
 * @param filename The selected file path.
 * @param fileDialog ImGui file browser instance.
 */
void conversionInfoPopup(char* filename, ImGui::FileBrowser& fileDialog);


/**
 * @brief Displays the mesh simplification popup window.
 * @param filename The selected file path.
 * @param resolution The resolution of the simplification.
 */
void simplificationOutOfCoreInfoPopup(char* filename, int &resolution);

/**
 * @brief Displays the mesh simplification popup window.
 * @param filename The selected file path.
 * @param resolution The resolution of the cutting.
 */
void simplificationInfoPopup(char* filename, int &resolution);


/**
 * @brief Loads a mesh from the given file path.
 * @param filename The path to the mesh file.
 * @param extension The file extension.
 */
void loadMesh(const char* filename, const std::string& extension);

#endif // UI_H