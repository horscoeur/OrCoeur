#include "ui.h"
#include "mesh_loader.h"
#include "imgui.h"
#include <iostream>
#include <cstring>

#include "mesh_conversion.h"
#include "mesh_cutting.h"
#include "mesh_simplification.h"
#include "adaptive/adaptive_mesh_cutting.h"
#include "adaptive/bsp_tree.h"
#include "adaptive/adaptive_mesh_simplification.h"
#include "polyscope/surface_mesh.h"

struct AdaptiveSimplificationOptions {
    int resolution = 100;
    int leafsCount = 100;
};

struct NormalSimplificationOptions {
    int resolution = 100;
};

static SimplificationMode simplificationMode = SIMPLIFICATION_MODE_NORMAL;
static bool simplifyTheMeshAfterFileSelection = false;
static char filename[2048] = "";


static AdaptiveSimplificationOptions adaptiveOptions;
static NormalSimplificationOptions normalOptions;
static std::vector<polyscope::PointCloud*> displayedPoints;
static ImGui::FileBrowser fileDialog;

void configureImGuiStyle() {
    ImGuiStyle *style = &ImGui::GetStyle();
    style->WindowRounding = 1;
    style->FrameRounding = 1;
    style->FramePadding.y = 4;
    style->ScrollbarRounding = 1;
    style->ScrollbarSize = 20;

    // Define UI colors
    ImVec4 *colors = style->Colors;
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.75f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
    colors[ImGuiCol_Border] = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.50f, 0.50f, 0.50f, 0.39f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.45f, 0.65f, 0.55f, 0.40f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.60f, 0.50f, 0.69f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.22f, 0.47f, 0.37f, 0.83f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.27f, 0.57f, 0.45f, 0.87f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.22f, 0.47f, 0.37f, 0.83f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.40f, 0.55f, 0.48f, 0.80f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.50f, 0.50f, 0.50f, 0.39f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.20f, 0.20f, 0.30f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.80f, 0.62f, 0.40f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab] = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.39f, 0.80f, 0.61f, 0.60f);
    colors[ImGuiCol_Button] = ImVec4(0.30f, 0.56f, 0.44f, 0.62f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.35f, 0.66f, 0.52f, 0.79f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.75f, 0.60f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.35f, 0.80f, 0.60f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.80f, 0.60f, 0.50f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.85f, 0.70f, 0.70f);
    colors[ImGuiCol_Separator] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.65f, 0.75f, 0.70f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.75f, 0.95f, 0.85f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.78f, 1.00f, 0.90f, 0.60f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.78f, 1.00f, 0.90f, 0.90f);
    colors[ImGuiCol_PlotLines] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_Tab] = ImVec4(0.22f, 0.47f, 0.37f, 0.83f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.30f, 0.62f, 0.48f, 0.83f);
    colors[ImGuiCol_TabActive] = ImVec4(0.35f, 0.72f, 0.54f, 0.83f);

    fileDialog.SetTitle("Open a mesh file");
    fileDialog.SetTypeFilters({".obj", ".ply"});
}

void handleFileSelection() {
    fileDialog.Display();

    if (fileDialog.HasSelected()) {
        std::strcpy(filename, fileDialog.GetSelected().string().c_str());

        if (simplifyTheMeshAfterFileSelection) {
            const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
            if (extension != ".obj" && extension != ".ply") {
                std::cerr << "Error: Unsupported file format." << std::endl;
            } else {

                // Perform the simplification
                if (simplificationMode == SIMPLIFICATION_MODE_NORMAL) {
                    std::string simplifiedMesh;
                    normalMeshSimplificationPipeline(simplifiedMesh);
                    loadMesh(simplifiedMesh.c_str(), ".obj");
                } else if (simplificationMode == SIMPLIFICATION_MODE_ADAPTIVE) {
                    std::string simplifiedMesh;
                    adaptiveMeshSimplificationPipeline(simplifiedMesh);
                    loadMesh(simplifiedMesh.c_str(), ".obj");
                }
            }
        } else {
            // Load the mesh
            std::cout << "Loading " << filename << "..." << std::endl;
            loadMesh(filename, fileDialog.GetSelected().extension().string());
        }
        fileDialog.ClearSelected();

        // Clear the displayed point clouds
        for (const auto point : displayedPoints) {
            polyscope::removePointCloud(point->name);
        }
        displayedPoints.clear();
    }
}

void loadMeshUI() {
    if (ImGui::Button("Open a mesh file")) {
        simplifyTheMeshAfterFileSelection = false;
        fileDialog.Open();
    }

    ImGui::SameLine();

    ImGui::BeginDisabled(filename[0] == '\0');
    if (ImGui::Button("Reload the mesh") && filename[0] != '\0') {
        std::cout << "Reloading " << filename << "..." << std::endl;
        loadMesh(filename, std::string(filename).substr(std::string(filename).find_last_of('.')));
    }
    ImGui::EndDisabled();
}

void conversionUI() {
    if (ImGui::CollapsingHeader("Mesh Conversion", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::BeginDisabled(filename[0] == '\0');

        // Button to convert the loaded mesh to binary OBJSoup format (binary)
        if (ImGui::Button("Convert the loaded mesh to binary OBJSoup format") && filename[0] != '\0') {
            const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
            const std::string newFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + ".bin";
            convertToObjSoup(newFilename);
        }

        // Button to convert the loaded mesh to text OBJSoup format (text)
        if (ImGui::Button("Convert the loaded mesh to text OBJSoup format") && filename[0] != '\0') {
            const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
            const std::string newFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + ".objs";
            convertToObjSoup(newFilename);
        }

        ImGui::EndDisabled();
    }
}

void normalMeshSimplificationUI() {
    if (ImGui::CollapsingHeader("Normal Mesh Simplification", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Text("Mesh simplification settings:");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::SliderInt("Grid Resolution##Normal", &normalOptions.resolution, 2, 2000);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        // Button to simplify the loaded mesh
        ImGui::BeginDisabled(filename[0] == '\0');
        if (ImGui::Button("Simplify the loaded mesh##Normal") && filename[0] != '\0') {
            std::string simplifiedMesh;
            normalMeshSimplificationPipeline(simplifiedMesh);
            loadMesh(simplifiedMesh.c_str(), ".obj");
        }
        ImGui::EndDisabled();

        // Button to open a mesh and simplify it before loading
        ImGui::SameLine();
        if (ImGui::Button("Simplify a mesh and load it##Normal")) {
            simplifyTheMeshAfterFileSelection = true;
            simplificationMode = SIMPLIFICATION_MODE_NORMAL;
            fileDialog.Open();
        }
    }
}

void adaptiveMeshSimplificationUI() {
    if (ImGui::CollapsingHeader("Adaptive Mesh Simplification", ImGuiTreeNodeFlags_DefaultOpen)) {

        ImGui::Text("Adaptive mesh simplification settings:");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
        ImGui::SliderInt("Grid Resolution##Adaptive", &adaptiveOptions.resolution, 10, 1000000);
        ImGui::SliderInt("Leafs Count", &adaptiveOptions.leafsCount, 10, 600000);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        ImGui::Text("Debugging options:");
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

        // Button to visualize the leafs
        ImGui::BeginDisabled(filename[0] == '\0');
        if (ImGui::SmallButton("Visualize the leafs") && filename[0] != '\0') {
            visualizeLeafs();
        }
        ImGui::EndDisabled();

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

        // Button to simplify the loaded mesh
        ImGui::BeginDisabled(filename[0] == '\0');
        if (ImGui::Button("Simplify the loaded mesh##Adaptive") && filename[0] != '\0') {
            std::string simplifiedMesh;
            adaptiveMeshSimplificationPipeline(simplifiedMesh);
            loadMesh(simplifiedMesh.c_str(), ".obj");
        }
        ImGui::EndDisabled();

        // Button to open a mesh and simplify it before loading
        ImGui::SameLine();
        if (ImGui::Button("Simplify a mesh and load it##Adaptive")) {
            simplifyTheMeshAfterFileSelection = true;
            simplificationMode = SIMPLIFICATION_MODE_ADAPTIVE;
            fileDialog.Open();
        }
    }
}

void normalMeshSimplificationPipeline(std::string &outputFilenameOBJ) {
    const std::string file = std::string(filename).substr(0, std::string(filename).find_last_of('.'));
    const std::string outputFilenameBinary = file + ".bin";
    const std::string outputFilenamePlaneEquation = file + "_planeEquation.bin";
    const std::string outputFilenamePlaneEquationSorted = file + "_planeEquationSorted.bin";
    const std::string outputFilenameRepresentatives = file + "_representatives.bin";
    const std::string outputFilenameTriangleCluster = file + "_triangleCluster.bin";
    const std::string outputFilenameSimplified = file + "_simplified.bin";
    outputFilenameOBJ = file + "_simplified.obj";

    // Convert the loaded mesh to binary OBJSoup format
    convertToObjSoup(outputFilenameBinary);

    // Perform the mesh cutting
    meshCutting(outputFilenameBinary, outputFilenamePlaneEquation, outputFilenameTriangleCluster, normalOptions.resolution);

    externalMergeSortGridPlaneEntry(outputFilenamePlaneEquation, outputFilenamePlaneEquationSorted);
    remove(outputFilenamePlaneEquation.c_str());

    computeGridCellRepresentatives(outputFilenamePlaneEquationSorted,  outputFilenameRepresentatives);
    remove(outputFilenamePlaneEquationSorted.c_str());

    generateSimplifiedMeshBin(outputFilenameRepresentatives, outputFilenameTriangleCluster, outputFilenameSimplified);
    remove(outputFilenameRepresentatives.c_str());
    remove(outputFilenameTriangleCluster.c_str());

    // Convert the simplified mesh to OBJ format
    convertOBJSoupToOBJ(outputFilenameSimplified, outputFilenameOBJ);
    remove(outputFilenameSimplified.c_str());
}

void adaptiveMeshSimplificationPipeline(std::string &outputFilenameOBJ) {
    const std::string file = std::string(filename).substr(0, std::string(filename).find_last_of('.'));
    const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));

    const std::string outputFilenameBinary = file + ".bin";
    const std::string outputFilenameSimplified = file + "_simplified.bin";
    outputFilenameOBJ = file + "_adaptive_simplified.obj";

    // Drop the points loaded
    for (const auto point : displayedPoints) {
        polyscope::removePointCloud(point->name);
    }
    displayedPoints.clear();

    convertToObjSoup(outputFilenameBinary);

    std::vector<CellData> cells = meshCuttingDualQuadric(outputFilenameBinary, adaptiveOptions.resolution);
    std::cout << "Mesh cutting completed. " << cells.size() << " cells generated." << std::endl;

    BSPNode* root = buildBSPTree(cells, adaptiveOptions.leafsCount);
    std::cout << "BSP tree built" << std::endl;

    // Free the cells
    cells = std::vector<CellData>();

    adaptiveMeshSimplification(outputFilenameBinary, outputFilenameSimplified, root);
    std::cout << "Mesh simplification completed." << std::endl;
    delete root;

    convertOBJSoupToOBJ(outputFilenameSimplified, outputFilenameOBJ);
    remove(outputFilenameSimplified.c_str());
}

void loadMesh(const char* filename, const std::string& extension) {
    // Free the previously loaded mesh
    polyscope::removeAllStructures();

    std::vector<std::array<float, 3>> vertices;
    std::vector<std::array<int, 3>> faces;

    if (extension == ".obj") {
        extractVerticesAndFacesFromOBJ(filename, vertices, faces);
        polyscope::registerSurfaceMesh("Loaded Mesh", vertices, faces);
    } else if (extension == ".ply") {
        extractVerticesAndFacesFromPLY(filename, vertices, faces);
        polyscope::registerSurfaceMesh("Loaded Mesh", vertices, faces);
    } else {
        std::cerr << "Error: Unsupported file format." << std::endl;
    }
}

void convertToObjSoup(const std::string &outputFilenameBinary) {
    const std::string file = std::string(filename).substr(0, std::string(filename).find_last_of('.'));
    const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));

    // Check if the file is already in OBJSoup format
    std::ifstream fileStream(outputFilenameBinary);
    if (!fileStream.is_open()) {
        if (extension == ".obj") {
            convertOBJtoOBJSoup(filename, outputFilenameBinary);
        } else if (extension == ".ply") {
            convertPLYtoOBJSoup(filename, outputFilenameBinary);
        } else {
            std::cerr << "Error: Unsupported file format." << std::endl;
        }
    } else {
        std::cout << "Using cached OBJSoup file: " << outputFilenameBinary << std::endl;
    }
}

void visualizeLeafs() {
    const std::string file = std::string(filename).substr(0, std::string(filename).find_last_of('.'));
    const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
    const std::string outputFilenameBinary = file + ".bin";

    // Drop the points loaded
    for (const auto point : displayedPoints) {
        polyscope::removePointCloud(point->name);
    }
    displayedPoints.clear();

    convertToObjSoup(outputFilenameBinary);
    const std::vector<CellData> cells = meshCuttingDualQuadric(outputFilenameBinary, adaptiveOptions.resolution);
    remove(outputFilenameBinary.c_str());

    BSPNode* root = buildBSPTree(cells, adaptiveOptions.leafsCount);
    root->plot(displayedPoints, 0.);
    delete root;
}