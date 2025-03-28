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
 */
void handleFileSelection(char* filename, ImGui::FileBrowser& fileDialog);

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


void streamSimplificationPopup();

/**
 * @brief Loads a mesh from the given file path.
 * @param filename The path to the mesh file.
 * @param extension The file extension.
 */
void loadMesh(const char* filename, const std::string& extension);

#endif // UI_H