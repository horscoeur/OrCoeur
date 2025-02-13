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

#endif // UI_H