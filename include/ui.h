#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "imfilebrowser.h"
#include "polyscope/point_cloud.h"

enum SimplificationMode {
    SIMPLIFICATION_MODE_NORMAL,
    SIMPLIFICATION_MODE_ADAPTIVE,
};


/**
 * @brief Configures the ImGui color scheme and style.
 */
void configureImGuiStyle();

void handleFileSelection();

void loadMeshUI();
void conversionUI();
void normalMeshSimplificationUI();
void adaptiveMeshSimplificationUI();

void normalMeshSimplificationPipeline(std::string &outputFilenameOBJ);
void adaptiveMeshSimplificationPipeline(std::string &outputFilenameOBJ);

// Debugging functions
void visualizeLeafs();

void convertToObjSoup(const std::string &outputFilenameBinary);

void streamSimplificationPopup();

/**
 * @brief Loads a mesh from the given file path.
 * @param filename The path to the mesh file.
 * @param extension The file extension.
 */
void loadMesh(const char* filename, const std::string& extension);

#endif // UI_H