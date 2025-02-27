#include "ui.h"
#include "mesh_loader.h"
#include "imgui.h"
#include <iostream>
#include <cstring>

#include "mesh_conversion.h"
#include "mesh_cutting.h"
#include "mesh_simplification.h"
#include "polyscope/surface_mesh.h"

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
}

void handleFileSelection(char* filename, ImGui::FileBrowser& fileDialog) {
    ImGui::PushItemWidth(100);
    if (ImGui::Button("Open a mesh file")) {
        fileDialog.Open();
    }
    ImGui::SameLine();
    if (ImGui::Button("Convert to OBJSoup")) {

        // If a file is loaded
        if (filename[0] != '\0') {
            const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
            if (extension != ".obj" && extension != ".ply") {
                std::cerr << "Error: Unsupported file format." << std::endl;
            } else {
                ImGui::OpenPopup("Mesh Conversion");
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cut mesh")) {
        // If a file is loaded
        if (filename[0] != '\0') {
            const std::string extension = std::string(filename).substr(std::string(filename).find_last_of('.'));
            if (extension != ".obj" && extension != ".ply") {
                std::cerr << "Error: Unsupported file format." << std::endl;
            } else {
                ImGui::OpenPopup("Mesh Cutting");
            }
        }
    }
    ImGui::PopItemWidth();

    fileDialog.Display();

    if (fileDialog.HasSelected()) {
        std::strcpy(filename, fileDialog.GetSelected().string().c_str());
        std::cout << "Loading " << filename << "..." << std::endl;

        // Load the mesh
        loadMesh(filename, fileDialog.GetSelected().extension().string());

        fileDialog.ClearSelected();
    }
}

void cuttingInfoPopup(char* filename, int &resolution) {
    if (ImGui::BeginPopupModal("Mesh Cutting", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The selected mesh will be cut into clusters.");
        ImGui::Text("Would you like to proceed?");


        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

        ImGui::InputInt("Resolution", &resolution);
        ImGui::SameLine();
        bool notCutTheMesh = false;
        if (ImGui::Button("See Grid", ImVec2(120, 0))) {
            notCutTheMesh = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Yes", ImVec2(120, 0)) || notCutTheMesh) {
            //Add input field for resolution
            const std::string newFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + ".bin";
            // If file already exists pass to the next step
            if (!std::ifstream(newFilename)){
                // Perform the conversion
                if (filename[std::strlen(filename) - 1] == 'j') {
                    convertOBJtoOBJSoup(filename, newFilename);
                } else {
                    convertPLYtoOBJSoup(filename, newFilename);
                }
            }

            std::string file = std::string(filename).substr(0, std::string(filename).find_last_of('.'));

            std::string outputFilenamePlaneEquation = file + "PlaneEquation.bin";
            std::string outputFilenamePlaneEquationSorted = file + "PlaneEquationSorted.bin";
            std::string outputFilenameTriangleCluster = file + "TriangleCluster.bin";
            std::string outputFilenameRepresentatives = file + "Representatives.bin";
            std::string outputFilenameSimplified = file + "Simplified.bin";
            std::string outputFilenameSimplifiedObj = file + "Simplified.txt";
            std::string outputFilenameOBJ = file + "Simplified.obj";

            // Perform the mesh cutting
            meshCutting(newFilename, outputFilenamePlaneEquation, outputFilenameTriangleCluster, resolution, !notCutTheMesh);

            // Sort the plane equations by grid index
            externalMergeSortGridPlaneEntry(outputFilenamePlaneEquation, outputFilenamePlaneEquationSorted);

            // Compute the representative vertices
            computeGridCellRepresentatives(outputFilenamePlaneEquationSorted,  outputFilenameRepresentatives);

            // Generate the simplified mesh by replacing the grid cells with their representative vertices
            int nbOfFaces = generateSimplifiedMeshBin(outputFilenameRepresentatives, outputFilenameTriangleCluster, outputFilenameSimplified);

            // Convert the simplified mesh to text format
            exportBinaryOBJSoupToText(outputFilenameSimplified, outputFilenameSimplifiedObj, nbOfFaces);

            // Convert the simplified mesh to OBJ format

            convertOBJSoupToOBJ(outputFilenameSimplifiedObj, outputFilenameOBJ);


            // Close the popup
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void conversionInfoPopup(char* filename, ImGui::FileBrowser& fileDialog) {
    if (ImGui::BeginPopupModal("Mesh Conversion", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("The selected mesh will be converted to OBJSoup format.");
        ImGui::Text("Would you like to proceed? (The original file will not be modified.)");

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
        ImGui::Separator();
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);

        if (ImGui::Button("Yes", ImVec2(120, 0))) {
            const std::string newFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + ".bin";

            // Perform the conversion
            if (filename[std::strlen(filename) - 1] == 'j') {
                convertOBJtoOBJSoup(filename, newFilename);
            } else {
                convertPLYtoOBJSoup(filename, newFilename);
            }

            // Close the popup and clear the selected file
            ImGui::CloseCurrentPopup();
            fileDialog.ClearSelected();
        }
        ImGui::SameLine();
        if (ImGui::Button("Yes (Text Version)", ImVec2(150, 0))) {
            const std::string newFilename = std::string(filename).substr(0, std::string(filename).find_last_of('.')) + ".objs";

            // Perform the conversion
            if (filename[std::strlen(filename) - 1] == 'j') {
                convertOBJtoOBJSoup(filename, newFilename);
            } else {
                convertPLYtoOBJSoup(filename, newFilename);
            }

            // Close the popup and clear the selected file
            ImGui::CloseCurrentPopup();
            fileDialog.ClearSelected();
        }
        ImGui::SetItemDefaultFocus();
        ImGui::SameLine();
        if (ImGui::Button("No", ImVec2(120, 0))) {
            ImGui::CloseCurrentPopup();
            fileDialog.ClearSelected();
        }
        ImGui::EndPopup();
    }
}

void loadMesh(const char* filename, const std::string& extension) {
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