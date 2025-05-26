#include <string>
#include <fstream>
#include <ostream>
#include <iostream>
#include <iomanip>
#include <vector>
#include <unistd.h>
#include <glm/glm.hpp>

#include "stream_simplification.h"
#include <polyscope/surface_mesh.h>
#include <mesh_loader.h>

#include "quadrics.h"

void AffichageDebug(StreamMeshData& meshData, int vertexA, int vertexB, int vertexC) {
    // Affichage des coordonnées dans le vertexMap en vert dans le terminal
    // affichage en vert
    std::cout << "\033[32m" << "Vertex Map : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.vertexMap) {
        if (pair.second == vertexA || pair.second == vertexB || pair.second == vertexC) {
            std::cout << "\033[32m" << "Vertex: (" << pair.first.x << ", " << pair.first.y << ", " << pair.first.z <<
                ") -> ID: " << pair.second << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets avec leur indice de triangle dans le triangleList en bleu
    std::cout << "\033[34m" << "Triangle List : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.triangleList) {
        if (pair.first == vertexA || pair.first == vertexB || pair.first == vertexC) {
            std::cout << "\033[34m" << "Vertex ID: " << pair.first << " -> Triangles: ";
            for (int triangleIndex : pair.second) {
                std::cout << triangleIndex << " ";
            }
            std::cout << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets avec leur liste de voisins dans le adjacencyList en rouge
    std::cout << "\033[31m" << "Adjacency List : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.adjacencyList) {
        if (pair.first == vertexA || pair.first == vertexB || pair.first == vertexC) {
            std::cout << "\033[31m" << "Vertex ID: " << pair.first << " -> Neighbors: ";
            for (int neighborIndex : pair.second) {
                std::cout << neighborIndex << " ";
            }
            std::cout << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets déjà vus dans le adjacencyListAlreadySeen en jaune
    std::cout << "\033[33m" << "Adjacency List Already Seen : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.adjacencyListAlreadySeen) {
        if (pair.first == vertexA || pair.first == vertexB || pair.first == vertexC) {
            std::cout << "\033[33m" << "Vertex ID: " << pair.first << " -> Neighbors: ";
            for (int neighborIndex : pair.second) {
                std::cout << neighborIndex << " ";
            }
            std::cout << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets déjà écrits dans le adjacencyListAlreadyWrote en bleu
    std::cout << "\033[34m" << "Adjacency List Already Wrote : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.adjacencyListAlreadyWrote) {
        if (pair.first == vertexA || pair.first == vertexB || pair.first == vertexC) {
            std::cout << "\033[34m" << "Vertex ID: " << pair.first << " -> Neighbors: ";
            for (int neighborIndex : pair.second) {
                std::cout << neighborIndex << " ";
            }
            std::cout << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets non dans le bord dans le vertexNotInBorder en magenta
    std::cout << "\033[35m" << "Vertex Not In Border : " << "\033[0m" << std::endl;
    for (int vertexIndex : meshData.vertexNotInBorder) {
        if (vertexIndex == vertexA || vertexIndex == vertexB || vertexIndex == vertexC) {
            std::cout << "\033[35m" << "Vertex ID: " << vertexIndex << "\033[0m" << std::endl;
        }
    }

    // Affichage des sommets dans le bord dans le vertexInBorderBC en jaune
    std::cout << "\033[33m" << "Vertex In Border BC : " << "\033[0m" << std::endl;
    for (int vertexIndex : meshData.vertexInBorderBC) {
        if (vertexIndex == vertexA || vertexIndex == vertexB || vertexIndex == vertexC) {
            std::cout << "\033[33m" << "Vertex ID: " << vertexIndex << "\033[0m" << std::endl;
        }
    }


    // Affichage des quadrics dans le triangleQuadricMap en cyan
    /*std::cout << "\033[36m" << "Triangle Quadric Map : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.triangleQuadricMap) {
        std::cout << "\033[36m" << "Triangle ID: " << pair.first << " -> Quadric: \n";
        //Sous forme de matrice 4x4
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                std::cout << std::setw(10) << pair.second(i, j) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << "\033[0m" << std::endl;
    }
    // Affichage inCoreTriangleBuffer en orange
    std::cout << "\033[38;5;214m" << "In Core Triangle Buffer : " << "\033[0m" << std::endl;
    for (const auto& pair : meshData.inCoreTriangleBuffer) {
        std::cout << "\033[38;5;214m" << "Triangle ID: " << pair.first << " -> Triangle: ";
        std::cout << "(" << pair.second.v1.x << ", " << pair.second.v1.y << ", " << pair.second.v1.z << ") ";
        std::cout << "(" << pair.second.v2.x << ", " << pair.second.v2.y << ", " << pair.second.v2.z << ") ";
        std::cout << "(" << pair.second.v3.x << ", " << pair.second.v3.y << ", " << pair.second.v3.z << ") ";
        std::cout << "\033[0m" << std::endl;
    }*/
}

bool findNeighborsOfVertexAWrite(int vertexA, int& vertexB, int& vertexC, StreamMeshData& meshData) {
    // On va chercher un voisin commun entre A et B, vu que les deux listes sont triées, on peut réduire le nombre de comparaisons
    std::vector<int>& adjacencyListVertexA = meshData.adjacencyListAlreadySeen[vertexA];
    std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[vertexB];
    std::vector<int> commonNeighbors;
    size_t i = 0, j = 0;
    while (i < adjacencyListVertexA.size() && j < adjacencyListVertexB.size()) {
        if (adjacencyListVertexA[i] == adjacencyListVertexB[j] && adjacencyListVertexA[i] != vertexB &&
            adjacencyListVertexA[i] != vertexC) {
            commonNeighbors.push_back(adjacencyListVertexA[i]);
            i++;
            j++;
        }
        else if (adjacencyListVertexA[i] < adjacencyListVertexB[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    if (commonNeighbors.size() == 0) {
        //std::cout << "No common neighbors found, try again" << std::endl;
        return false;
    }

    // On va vérifier si le voisin commun est pas dans la border (J'ai décidé de ne pas verifier si il est simplifié ou pas)
    for (int i = 0; i < commonNeighbors.size(); i++) {
        int neighbor = commonNeighbors[i];
        for (int j = 0; j < meshData.vertexNotInBorder.size(); j++) {
            if (meshData.vertexNotInBorder[j].vertexId == neighbor) {
                // && meshData.vertexNotInBorder[j].isSimplified // Depends de si on veut un voisin simplifié ou pas
                vertexC = neighbor;
                break;
            }
        }
    }

    if (vertexC == -1) {
        //std::cout << "No common neighbor not in border AB found , try again." << std::endl;
        return false;
    }
    return true;
}

bool initializeBorderBC(int& chosenOneVertexA, int& chosenOneVertexB, int& chosenOneVertexC, StreamMeshData& meshData) {
    // Pour l'instant on va sélectionner le premier sommets de la liste qui a été simplifié dans vertexNotInBorder
    if (meshData.vertexNotInBorder.size() == 0) {
        //std::cout << "No vertices in vertexNotInBorder, end initialisation." << std::endl;
        return false;
    }

    // On va faire une copie de vertexNotInBorder pour ne pas la modifier
    std::vector<vertexAlreadyBeSimplify> vertexNotInBorderCopy = meshData.vertexNotInBorder;
    // Choix aléatoire de A : (TODO: on pourrait faire un meilleur choix avec les quadrics peut être)
    vertexAlreadyBeSimplify vertexA = vertexNotInBorderCopy[rand() % vertexNotInBorderCopy.size()];
    while (vertexNotInBorderCopy.size() > 1 && !vertexA.isSimplified) {
        vertexNotInBorderCopy.erase(std::remove(vertexNotInBorderCopy.begin(), vertexNotInBorderCopy.end(), vertexA),
                                    vertexNotInBorderCopy.end());
        vertexA = vertexNotInBorderCopy[rand() % vertexNotInBorderCopy.size()];
    }

    if (!vertexA.isSimplified) {
        //std::cout << "Vertex A is not simplified, exiting function." << std::endl;
        return false;
    }

    // On va chercher un voisin de A qui est pas dans la border et qui est simplifié
    std::vector<int>& adjacencyListVertexA = meshData.adjacencyListAlreadySeen[vertexA.vertexId];
    std::vector<int> potentialVertexB;

    for (int i = 0; i < adjacencyListVertexA.size(); i++) {
        int neighbor = adjacencyListVertexA[i];
        for (int j = 0; j < meshData.vertexNotInBorder.size(); j++) {
            if (meshData.vertexNotInBorder[j].vertexId == neighbor && meshData.vertexNotInBorder[j].isSimplified) {
                potentialVertexB.push_back(neighbor);
            }
        }
    }
    if (potentialVertexB.empty()) {
        //std::cout << "No neighbor found for vertex A, exiting function." << std::endl;
        return false;
    }

    int vertexB = -1, vertexC = -1;
    vertexB = potentialVertexB[rand() % potentialVertexB.size()];
    potentialVertexB.erase(std::remove(potentialVertexB.begin(), potentialVertexB.end(), vertexB),
                           potentialVertexB.end());
    while (potentialVertexB.size() > 1 && !findNeighborsOfVertexAWrite(vertexA.vertexId, vertexB, vertexC, meshData)) {
        potentialVertexB.erase(std::remove(potentialVertexB.begin(), potentialVertexB.end(), vertexB),
                               potentialVertexB.end());
        vertexB = potentialVertexB[rand() % potentialVertexB.size()];
    }
    if (vertexC == -1) {
        //std::cout << "No neighbor found for vertex B, exiting function." << std::endl;
        return false;
    }

    // On a nos trois sommets, on va les rajouter dans vertexInBorderBC
    meshData.vertexInBorderBC.push_back(vertexA);
    meshData.vertexInBorderBC.push_back(vertexB);
    meshData.vertexInBorderBC.push_back(vertexC);

    // On va remplir les maps de chaque sommet concernant adjacencyListAlreadyWrote
    //Sommet A
    meshData.adjacencyListAlreadyWrote[vertexA].push_back(vertexB);
    meshData.adjacencyListAlreadyWrote[vertexA].push_back(vertexC);
    insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexA]);
    // Sommet B
    meshData.adjacencyListAlreadyWrote[vertexB].push_back(vertexA);
    meshData.adjacencyListAlreadyWrote[vertexB].push_back(vertexC);
    insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexB]);
    // Sommet C
    meshData.adjacencyListAlreadyWrote[vertexC].push_back(vertexA);
    meshData.adjacencyListAlreadyWrote[vertexC].push_back(vertexB);
    insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexC]);


    // On va s'occuper de la mise à jour des listes d'adjacence
    std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[vertexB];
    std::vector<int>& adjacencyListVertexC = meshData.adjacencyListAlreadySeen[vertexC];

    // Lambda function to delete vertex from adjacency list
    auto deleteVertexFromAdjacencyList = [&](int vertex1, int vertex2, std::vector<int>& adjacencyListAlreadySeen) {
        for (int i = 0; i < adjacencyListAlreadySeen.size(); i++) {
            int neighbor = adjacencyListAlreadySeen[i];
            if (neighbor == vertex1 || neighbor == vertex2) {
                adjacencyListAlreadySeen.erase(adjacencyListAlreadySeen.begin() + i);
                i--;
            }
        }
    };

    // On va supprimer les sommets C et B de la liste d'adjacence déjà vue de A.
    deleteVertexFromAdjacencyList(vertexB, vertexC, adjacencyListVertexA);
    // On va supprimer les sommets A et C de la liste d'adjacence déjà vue de B.
    deleteVertexFromAdjacencyList(vertexA, vertexC, adjacencyListVertexB);
    // On va supprimer les sommets A et B de la liste d'adjacence déjà vue de C.
    deleteVertexFromAdjacencyList(vertexA, vertexB, adjacencyListVertexC);

    chosenOneVertexA = vertexA.vertexId;
    chosenOneVertexB = vertexB;
    chosenOneVertexC = vertexC;

    // Et on va les supprimer de vertexNotInBorder car ils sont immuables maintenant
    auto deleteFromVertexNotInBorder = [&](int vertexId) {
        for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
            if (meshData.vertexNotInBorder[i].vertexId == vertexId) {
                meshData.vertexNotInBorder.erase(meshData.vertexNotInBorder.begin() + i);
                break;
            }
        }
    };
    // On va supprimer les sommets A, B et C de vertexNotInBorder
    deleteFromVertexNotInBorder(chosenOneVertexA);
    deleteFromVertexNotInBorder(chosenOneVertexB);
    deleteFromVertexNotInBorder(chosenOneVertexC);

    return true;
}

bool getNeighborHighestScoreOfAAndB(int VertexA, int VertexB, int& chosenOneVertexA, int& chosenOneVertexB,
                                    int& chosenOneVertexC, float& errorTest, int& nbCandidate, bool& CfindInBorderBC,
                                    StreamMeshData& meshData) {
    bool weHaveGetAHighestScore = false;
    // Tout ca représente un test pour récupérer le meilleur score parmi les voisins de A et B
    std::vector<int>& adjacencyListVertexA = meshData.adjacencyListAlreadySeen[VertexA];
    std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[VertexB];
    std::vector<int> commonNeighbors;
    std::vector<int> commonNeighborsInBorderBC;
    size_t i = 0, j = 0;
    while (i < adjacencyListVertexA.size() && j < adjacencyListVertexB.size()) {
        // Il faut que ce sommets en commun soit également hors bordure
        if (adjacencyListVertexA[i] == adjacencyListVertexB[j] && adjacencyListVertexA[i] != VertexA &&
            adjacencyListVertexA[i] != VertexB) {
            int commonNeighbor = adjacencyListVertexA[i];
            bool isOutBorder = false;
            bool isInBorderBC = false;
            for (int k = 0; k < meshData.vertexNotInBorder.size(); k++) {
                if (meshData.vertexNotInBorder[k].vertexId == commonNeighbor) {
                    isOutBorder = true;
                    break;
                }
            }
            if (isOutBorder) {
                commonNeighbors.push_back(adjacencyListVertexA[i]);
            }
            else {
                for (int k = 0; k < meshData.vertexInBorderBC.size(); k++) {
                    if (meshData.vertexInBorderBC[k] == commonNeighbor) {
                        isInBorderBC = true;
                        break;
                    }
                }
                if (isInBorderBC) {
                    commonNeighborsInBorderBC.push_back(adjacencyListVertexA[i]);
                }
            }
            i++;
            j++;
        }
        else if (adjacencyListVertexA[i] < adjacencyListVertexB[j]) {
            i++;
        }
        else {
            j++;
        }
    }
    if (commonNeighbors.size() == 0 && commonNeighborsInBorderBC.size() == 0) {
        //std::cout << "No common neighbors found, try again" << std::endl;
        return false;
    }

    auto evaluateErrorCommonNeighbor = [&](int neighbor) {
        bool itsABetterCandidate = false;
        if (!meshData.adjacencyList[neighbor].empty()) {
            //std::cout << "There is still neighbor in border AC" << std::endl;
            return false;
        }
        // On va chercher le quad du triangle grâce aux trois coordonnées
        std::vector<int>& trianglesVertexA = meshData.triangleList[VertexA];
        std::vector<int>& trianglesVertexB = meshData.triangleList[VertexB];
        std::vector<int>& trianglesVertexC = meshData.triangleList[neighbor];
        int commonTriangle = -1;
        // Loops through the triangles (they need to be sorted)
        size_t a = 0, b = 0, c = 0;
        while (a < trianglesVertexA.size() && b < trianglesVertexB.size() && c < trianglesVertexC.size()) {
            // If the common triangle is found, remove it from the triangle list of vertexA, vertexB and vertexC
            if (trianglesVertexB[b] == trianglesVertexA[a] && trianglesVertexA[a] == trianglesVertexC[c]) {
                commonTriangle = trianglesVertexB[b];
                break;
            }
            else {
                // If the common triangle is not found, find the minimum value of the three triangle lists and increment the corresponding index
                int minVal = std::min({trianglesVertexB[b], trianglesVertexA[a], trianglesVertexC[c]});
                if (trianglesVertexA[a] == minVal) ++a;
                if (trianglesVertexB[b] == minVal) ++b;
                if (trianglesVertexC[c] == minVal) ++c;
            }
        }
        if (commonTriangle == -1) {
            // std::cout << "Erreur : No common triangle found, exiting function of getNeighborHighestScoreOfAAndB." <<std::endl;
            return false;
        }
        //std::cout << "Triangle ID : " << commonTriangle << std::endl;
        TriangleCoordinates triangle = meshData.inCoreTriangleBuffer[commonTriangle];

        Vertex triangleVertexA = triangle.v1;
        Vertex triangleVertexB = triangle.v2;
        Vertex triangleVertexC = triangle.v3;

        Vertex triangleCenter = {
            (triangleVertexA.x + triangleVertexB.x + triangleVertexC.x) / 3,
            (triangleVertexA.y + triangleVertexB.y + triangleVertexC.y) / 3,
            (triangleVertexA.z + triangleVertexB.z + triangleVertexC.z) / 3
        };

        Quadric triangleQuadric = meshData.triangleQuadricMap[VertexA];
        triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[VertexB]);
        triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[neighbor]);
        float currentError = evaluateError(triangleQuadric, triangleCenter);
        if (currentError > errorTest) {
            errorTest = currentError;
            chosenOneVertexC = neighbor;
            chosenOneVertexB = VertexB;
            chosenOneVertexA = VertexA;
            weHaveGetAHighestScore = true;
            itsABetterCandidate = true;
        }
        nbCandidate--;
        return itsABetterCandidate;
    };
    // On va vérifier lequel des voisins communs dans la bordure BC nous donne le meilleur score
    while (!commonNeighborsInBorderBC.empty() && nbCandidate > 0) {
        int neighbor = commonNeighborsInBorderBC[0];
        commonNeighborsInBorderBC.erase(commonNeighborsInBorderBC.begin());
        if (evaluateErrorCommonNeighbor(neighbor)) {
            CfindInBorderBC = true;
        }
    }
    // On va vérifier lequel des voisins communs nous donne le meilleur score
    while (!commonNeighbors.empty() && nbCandidate > 0) {
        int neighbor = commonNeighbors[0];
        commonNeighbors.erase(commonNeighbors.begin());
        if (evaluateErrorCommonNeighbor(neighbor)) {
            CfindInBorderBC = false;
        }
    }
    return weHaveGetAHighestScore;
}

bool chooseATriangle(int& chosenOneVertexA, int& chosenOneVertexB, int& chosenOneVertexC, float& error,
                     int& nbCandidate, bool& BfindInBorderBC, bool& CfindInBorderBC, StreamMeshData& meshData) {
    bool weFindATriangle = false;
    std::vector<int> vertexInBorderBC = meshData.vertexInBorderBC;

    while (!vertexInBorderBC.empty() && nbCandidate > 0) {
        int indiceVertexA = rand() % vertexInBorderBC.size();
        int VertexA = vertexInBorderBC[indiceVertexA];
        vertexInBorderBC.erase(vertexInBorderBC.begin() + indiceVertexA);
        //std::cout << "Vertex A : " << VertexA << std::endl;
        if (!meshData.adjacencyList[VertexA].empty()) {
            //std::cout << "There is still neighbor in border AC" << std::endl;
            continue;
        }
        // Si il y a plus qu'un triangle associé a A
        if (meshData.adjacencyListAlreadySeen[VertexA].empty() && meshData.adjacencyListAlreadyWrote[VertexA].size() ==
            2) {
            int VertexB = meshData.adjacencyListAlreadyWrote[VertexA][0];
            if (!meshData.adjacencyList[VertexB].empty()) {
                //std::cout << "There is still neighbor in border AC" << std::endl;
                continue;
            }
            int VertexC = meshData.adjacencyListAlreadyWrote[VertexA][1];
            if (!meshData.adjacencyList[VertexC].empty()) {
                //std::cout << "There is still neighbor in border AC" << std::endl;
                continue;
            }
            // Recherche de ce triangle dans les listes d'adjacence de triangles
            std::vector<int>& trianglesVertexA = meshData.triangleList[VertexA];
            std::vector<int>& trianglesVertexB = meshData.triangleList[VertexB];
            std::vector<int>& trianglesVertexC = meshData.triangleList[VertexC];
            int commonTriangle = -1;
            // Loops through the triangles (they need to be sorted)
            size_t a = 0, b = 0, c = 0;
            while (a < trianglesVertexA.size() && b < trianglesVertexB.size() && c < trianglesVertexC.size()) {
                // If the common triangle is found, remove it from the triangle list of vertexA, vertexB and vertexC
                if (trianglesVertexB[b] == trianglesVertexA[a] && trianglesVertexA[a] == trianglesVertexC[c]) {
                    commonTriangle = trianglesVertexB[b];
                    break;
                }
                else {
                    // If the common triangle is not found, find the minimum value of the three triangle lists and increment the corresponding index
                    int minVal = std::min({trianglesVertexB[b], trianglesVertexA[a], trianglesVertexC[c]});
                    if (trianglesVertexA[a] == minVal) ++a;
                    if (trianglesVertexB[b] == minVal) ++b;
                    if (trianglesVertexC[c] == minVal) ++c;
                }
            }
            if (commonTriangle == -1) {
                //std::cout << "No common triangle found, exiting function with vertex A : " << VertexA<< " and vertex B : " << VertexB << " and vertex C : " << VertexC << std::endl;
                continue;
            }
            //std::cout << "Triangle ID : " << commonTriangle << std::endl;
            TriangleCoordinates triangle = meshData.inCoreTriangleBuffer[commonTriangle];
            Vertex triangleVertexA = triangle.v1;
            Vertex triangleVertexB = triangle.v2;
            Vertex triangleVertexC = triangle.v3;

            Vertex triangleCenter = {
                (triangleVertexA.x + triangleVertexB.x + triangleVertexC.x) / 3,
                (triangleVertexA.y + triangleVertexB.y + triangleVertexC.y) / 3,
                (triangleVertexA.z + triangleVertexB.z + triangleVertexC.z) / 3
            };

            Quadric triangleQuadric = meshData.triangleQuadricMap[VertexA];
            triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[VertexB]);
            triangleQuadric = addQuadric(triangleQuadric, meshData.triangleQuadricMap[VertexC]);
            float currentError = evaluateError(triangleQuadric, triangleCenter);
            if (currentError > error) {
                error = currentError;
                chosenOneVertexC = VertexC;
                chosenOneVertexB = VertexB;
                chosenOneVertexA = VertexA;
                BfindInBorderBC = true;
                CfindInBorderBC = true;
                weFindATriangle = true;
            }
            nbCandidate--;
        }
        else {
            // On va d'abord testé ses sommets qui sont alreadyWrote et récupérer le sommet qui nous donnerais le meilleur score pour l'écriture
            std::vector<int> adjacencyListAlreadyWroteA = meshData.adjacencyListAlreadyWrote[VertexA];
            int VertexB = -1;
            // Tant qu'on a pas atteint la limite du nombre de candidats et que nore liste adjacencyListAlreadyWroteA n'est pas vide
            while (!adjacencyListAlreadyWroteA.empty() && nbCandidate > 0) {
                VertexB = adjacencyListAlreadyWroteA[0];
                adjacencyListAlreadyWroteA.erase(adjacencyListAlreadyWroteA.begin());
                //std::cout << "Test with Vertex B : " << VertexB << std::endl;
                if (!meshData.adjacencyList[VertexB].empty()) {
                    //std::cout << "There is still neighbor in border AC" << std::endl;
                    continue;
                }
                if (getNeighborHighestScoreOfAAndB(VertexA, VertexB, chosenOneVertexA, chosenOneVertexB,
                                                   chosenOneVertexC, error, nbCandidate, CfindInBorderBC, meshData)) {
                    //std::cout << "Found a better candidate in border BC with Vertex B : " << VertexB << std::endl;
                    BfindInBorderBC = true;
                    weFindATriangle = true;
                }
            }
            // On va faire le meme test, mais avec les vosins de A qui sont dans adjacencyListAlreadySeen
            std::vector<int> adjacencyListAlreadySeenA = meshData.adjacencyListAlreadySeen[VertexA];
            while (!adjacencyListAlreadySeenA.empty() && nbCandidate > 0) {
                VertexB = adjacencyListAlreadySeenA[0];
                adjacencyListAlreadySeenA.erase(adjacencyListAlreadySeenA.begin());
                //std::cout << "Test with Vertex B : " << VertexB << std::endl;
                if (!meshData.adjacencyList[VertexB].empty()) {
                    //std::cout << "There is still neighbor in border AC" << std::endl;
                    continue;
                }
                // On va vérifier si le voisin de A est pas dans la border
                bool testIsVertexInBorderAC = false;
                for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
                    if (meshData.vertexNotInBorder[i].vertexId == VertexB) {
                        testIsVertexInBorderAC = true;
                        break;
                    }
                }
                // Et si il est dans la borderBC
                bool testIsInVertexInBorderBC = false;
                for (int i = 0; i < meshData.vertexInBorderBC.size(); i++) {
                    if (meshData.vertexInBorderBC[i] == VertexB) {
                        testIsInVertexInBorderBC = true;
                        break;
                    }
                }
                //std::cout << "Test if Vertex B is in border BC : " << testIsInVertexInBorderBC << std::endl;
                //std::cout << "Test if Vertex B is in border AC : " << testIsVertexInBorderAC << std::endl;
                if (testIsVertexInBorderAC && !testIsInVertexInBorderBC) {
                    //std::cout << "Vertex B is in border AC and not in BC, we skip it." << std::endl;
                    continue;
                }
                // Si on trouve meilleur ici, c'est que le voisin de A est pas dans la border BC
                if (getNeighborHighestScoreOfAAndB(VertexA, VertexB, chosenOneVertexA, chosenOneVertexB,
                                                   chosenOneVertexC, error, nbCandidate, CfindInBorderBC, meshData)) {
                    //std::cout << "Found a better candidate not in border BC with Vertex B : " << VertexB << std::endl;
                    BfindInBorderBC = false;
                    weFindATriangle = true;
                }
            }
        }
    }

    if (chosenOneVertexB == -1 || chosenOneVertexC == -1 || chosenOneVertexA == -1 || (vertexInBorderBC.empty() &&
        nbCandidate > 0)) {
        while (nbCandidate > 0 && !initializeBorderBC(chosenOneVertexA, chosenOneVertexB, chosenOneVertexC, meshData)) {
            nbCandidate--;
        }
    }


    if (chosenOneVertexB == -1 || chosenOneVertexC == -1 || chosenOneVertexA == -1 ||
        !meshData.adjacencyList[chosenOneVertexA].empty() || !meshData.adjacencyList[chosenOneVertexB].empty() || !
        meshData.adjacencyList[chosenOneVertexC].empty()) {
        //std::cout << "No triangle to write found exit function." << std::endl;
        return false;
    }
    //std::cout << "Vertex A : " << chosenOneVertexA << std::endl;
    //std::cout << "Vertex B : " << chosenOneVertexB << std::endl;
    //std::cout << "Vertex C : " << chosenOneVertexC << std::endl;
    return weFindATriangle;
}

void deleteAndWriteThisTriangle(int chosenOneVertexA, int chosenOneVertexB, int chosenOneVertexC,
                                StreamMeshData& meshData, std::ofstream& outputFile, int* trianglesWritten) {
    // Récupération du triangle à partir de ces trois sommets
    std::vector<int>& trianglesVertexA = meshData.triangleList[chosenOneVertexA];
    std::vector<int>& trianglesVertexB = meshData.triangleList[chosenOneVertexB];
    std::vector<int>& trianglesVertexC = meshData.triangleList[chosenOneVertexC];
    int commonTriangle = -1;
    TriangleCoordinates commonTriangleCoord;
    // Loops through the triangles (they need to be sorted)
    size_t a = 0, b = 0, c = 0;
    while (a < trianglesVertexA.size() && b < trianglesVertexB.size() && c < trianglesVertexC.size()) {
        // If the common triangle is found, remove it from the triangle list of vertexA, vertexB and vertexC
        if (trianglesVertexB[b] == trianglesVertexA[a] && trianglesVertexA[a] == trianglesVertexC[c]) {
            commonTriangleCoord = meshData.inCoreTriangleBuffer[trianglesVertexB[b]];
            commonTriangle = trianglesVertexB[b];
            // On va quand meme l'effacer de InCore vu qu'il va être écrit
            meshData.inCoreTriangleBuffer.erase(trianglesVertexB[b]);
            //std::cout << " On supprime normalement Triangle ID : " << commonTriangle <<
            //" Des listes de triangles de A : " << chosenOneVertexA << " B : " << chosenOneVertexB << " C : " <<
            //chosenOneVertexC << std::endl;
            trianglesVertexB.erase(trianglesVertexB.begin() + b);
            trianglesVertexA.erase(trianglesVertexA.begin() + a);
            trianglesVertexC.erase(trianglesVertexC.begin() + c);
            break;
        }
        else {
            // If the common triangle is not found, find the minimum value of the three triangle lists and increment the corresponding index
            int minVal = std::min({trianglesVertexB[b], trianglesVertexA[a], trianglesVertexC[c]});
            if (trianglesVertexA[a] == minVal) ++a;
            if (trianglesVertexB[b] == minVal) ++b;
            if (trianglesVertexC[c] == minVal) ++c;
        }
    }
    if (commonTriangle == -1) {
        //AffichageDebug(meshData, chosenOneVertexA, chosenOneVertexB, chosenOneVertexC);
        std::cout << "\033[31mError: No common triangle found, exiting function.\033[0m" << std::endl;
        //throw std::runtime_error("No common triangle found"); // On throw une exception pour signaler l'erreur car
        // maintenant nos listes au dessus ne sont plus les mêmes
    } // donc le maillage est cassé
    //std::cout << "Triangle ID : " << commonTriangle << std::endl;
    outputFile.write(reinterpret_cast<char*>(&commonTriangleCoord), sizeof(commonTriangleCoord));

    // Lambda function to verify and delete vertex
    auto verifyAndDeleteVertexWrote = [&](int vertexId, std::vector<int>& adjacencyListVertex,
                                          std::vector<int>& adjacencyListVertexAlreadySeen,
                                          std::vector<int>& adjacencyListVertexAlreadyWrote) {
        if (adjacencyListVertexAlreadySeen.empty() && adjacencyListVertexAlreadyWrote.empty()) {
            if (!adjacencyListVertex.empty()) {
                std::cout << "\033[31mError: Vertex " << vertexId << " has neighbors in adjacencyList\033[0m" <<
                    std::endl;
                //throw std::runtime_error("He has neighbors in adjacencyList");
            }
            // supprime de vertexInBorderBC
            for (int i = 0; i < meshData.vertexInBorderBC.size(); i++) {
                if (meshData.vertexInBorderBC[i] == vertexId) {
                    meshData.vertexInBorderBC.erase(meshData.vertexInBorderBC.begin() + i);
                    break;
                }
            }
            // Supprime de ListAdjacency
            meshData.adjacencyList.erase(vertexId);
            // Supprime de ListAdjacencyAlreadySeen
            meshData.adjacencyListAlreadySeen.erase(vertexId);
            // Supprime de ListAdjacencyAlreadyWrote
            meshData.adjacencyListAlreadyWrote.erase(vertexId);
            // supprime de vertexMap
            for (auto it = meshData.vertexMap.begin(); it != meshData.vertexMap.end(); ++it) {
                if (it->second == vertexId) {
                    meshData.vertexMap.erase(it);
                    break;
                }
            }
            // supprime de triangleList
            if (meshData.triangleList[vertexId].size() != 0) {
                //AffichageDebug(meshData, vertexId, chosenOneVertexB, chosenOneVertexC);
                // affichage en rouge
                std::cout << "\033[31mError: Triangle list is not empty after removing vertex\033[0m" << std::endl;
                //throw std::runtime_error("Triangle list is not empty after removing vertex");
            }
            meshData.triangleList.erase(vertexId);
            // supprime de triangleQuadricMap
            meshData.triangleQuadricMap.erase(vertexId);
        }
    };

    std::vector<int>& adjacencyListAlreadySeenChosenA = meshData.adjacencyListAlreadySeen[chosenOneVertexA];
    std::vector<int>& adjacencyListAlreadySeenChosenB = meshData.adjacencyListAlreadySeen[chosenOneVertexB];
    std::vector<int>& adjacencyListAlreadySeenChosenC = meshData.adjacencyListAlreadySeen[chosenOneVertexC];

    std::vector<int>& adjacencyListVertexAlreadyWroteChosenA = meshData.adjacencyListAlreadyWrote[chosenOneVertexA];
    std::vector<int>& adjacencyListVertexAlreadyWroteChosenB = meshData.adjacencyListAlreadyWrote[chosenOneVertexB];
    std::vector<int>& adjacencyListVertexAlreadyWroteChosenC = meshData.adjacencyListAlreadyWrote[chosenOneVertexC];

    verifyAndDeleteVertexWrote(chosenOneVertexA, meshData.adjacencyList[chosenOneVertexA],
                               adjacencyListAlreadySeenChosenA, adjacencyListVertexAlreadyWroteChosenA);
    verifyAndDeleteVertexWrote(chosenOneVertexB, meshData.adjacencyList[chosenOneVertexB],
                               adjacencyListAlreadySeenChosenB, adjacencyListVertexAlreadyWroteChosenB);
    verifyAndDeleteVertexWrote(chosenOneVertexC, meshData.adjacencyList[chosenOneVertexC],
                               adjacencyListAlreadySeenChosenC, adjacencyListVertexAlreadyWroteChosenC);
    trianglesWritten += 1;
}

bool writeOneEdge(std::ofstream& outputFile, int nbCandidateTotal, int* trianglesWritten, StreamMeshData& meshData) {
    //std::cout << "Testing " << nbCandidateTotal << " candidate to outputfile " << std::endl;
    int nbCandidate = nbCandidateTotal; // TODO : A changer
    float error = -1e10;
    int chosenOneVertexA = -1, chosenOneVertexB = -1, chosenOneVertexC = -1;

    // Si la liste des sommets adjacents déjà écrits est vide, on va prendre un sommet aléatoire dans vertexNotInBorder
    if (meshData.vertexInBorderBC.size() == 0) {
        if (!initializeBorderBC(chosenOneVertexA, chosenOneVertexB, chosenOneVertexC, meshData)) {
            //std::cout << "Error: initializeBorderBC failed, exiting function." << std::endl;
            return false;
        }
    }
    else {
        // On va prendre un sommet aléatoire parmi les sommets de vertexInBorderBC
        bool BfindInBorderBC = false;
        bool CfindInBorderBC = false;
        if (!chooseATriangle(chosenOneVertexA, chosenOneVertexB, chosenOneVertexC, error, nbCandidate, BfindInBorderBC,
                             CfindInBorderBC, meshData)) {
            //std::cout << "ChooseATriangle failed, exiting function." << std::endl;
            return false;
        }

        // Si nos deux sommets adjacents sont dans la bordure
        if (BfindInBorderBC && CfindInBorderBC) {
            //std::cout << "Both vertices B and C are in border BC." << std::endl;

            auto moveVertexFromAdjacencyListAlreadySeenToAlreadyWrote = [&](int vertex1, int vertex2,
                                                                            std::vector<int>& adjacencyListAlreadySeen,
                                                                            std::vector<int>&
                                                                            adjacencyListAlreadyWrote) {
                for (int i = 0; i < adjacencyListAlreadySeen.size(); i++) {
                    int neighbor = adjacencyListAlreadySeen[i];
                    if (neighbor == vertex1 || neighbor == vertex2) {
                        adjacencyListAlreadyWrote.push_back(neighbor);
                        insertionSort(adjacencyListAlreadyWrote);
                        adjacencyListAlreadySeen.erase(adjacencyListAlreadySeen.begin() + i);
                        i--;
                    }
                }
            };

            auto deleteVertexFromAdjacencyListAlreadyWrote = [&](int vertex1, int vertex2,
                                                                 std::vector<int>& adjacencyListAlreadyWrote) {
                bool weHaveDeleteVertex1 = false;
                bool weHaveDeleteVertex2 = false;
                for (int i = 0; i < adjacencyListAlreadyWrote.size(); i++) {
                    int neighbor = adjacencyListAlreadyWrote[i];
                    if (neighbor == vertex1) {
                        adjacencyListAlreadyWrote.erase(adjacencyListAlreadyWrote.begin() + i);
                        weHaveDeleteVertex1 = true;
                        i--;
                    }
                    if (neighbor == vertex2) {
                        adjacencyListAlreadyWrote.erase(adjacencyListAlreadyWrote.begin() + i);
                        weHaveDeleteVertex2 = true;
                        i--;
                    }
                }
                return weHaveDeleteVertex1 && weHaveDeleteVertex2;
            };

            std::vector<int>& adjacencyListAlreadyWroteVertexA = meshData.adjacencyListAlreadyWrote[chosenOneVertexA];
            std::vector<int>& adjacencyListAlreadyWroteVertexB = meshData.adjacencyListAlreadyWrote[chosenOneVertexB];
            std::vector<int>& adjacencyListAlreadyWroteVertexC = meshData.adjacencyListAlreadyWrote[chosenOneVertexC];

            // on peut supprimer B et C de la liste d'adjacence déjà écrite de A
            if (!deleteVertexFromAdjacencyListAlreadyWrote(chosenOneVertexB, chosenOneVertexC,
                                                           adjacencyListAlreadyWroteVertexA)) {
                //std::cout << "Error: VERTEX A Failed one of the neighbors wasn't here in adjacencyListAlreadyWrote." <<std::endl;
            }
            // De même pour B
            if (!deleteVertexFromAdjacencyListAlreadyWrote(chosenOneVertexA, chosenOneVertexC,
                                                           adjacencyListAlreadyWroteVertexB)) {
                //std::cout << "Error: VERTEX B Failed one of the neighbors wasn't here in adjacencyListAlreadyWrote." <<std::endl;
            }
            // De même pour C
            if (!deleteVertexFromAdjacencyListAlreadyWrote(chosenOneVertexA, chosenOneVertexB,
                                                           adjacencyListAlreadyWroteVertexC)) {
                //std::cout << "Error: VERTEX C Failed one of the neighbors wasn't here in adjacencyListAlreadyWrote." <<std::endl;
            }

            std::vector<int>& adjacencyListVertexA = meshData.adjacencyListAlreadySeen[chosenOneVertexA];
            std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[chosenOneVertexB];
            std::vector<int>& adjacencyListVertexC = meshData.adjacencyListAlreadySeen[chosenOneVertexC];

            // Si un des sommets était présent dans la liste d'adjacence déjà vue de A, on va le déplacer dans la liste d'adjacence déjà écrite
            moveVertexFromAdjacencyListAlreadySeenToAlreadyWrote(chosenOneVertexB, chosenOneVertexC,
                                                                 adjacencyListVertexA,
                                                                 adjacencyListAlreadyWroteVertexA);
            // De même pour B
            moveVertexFromAdjacencyListAlreadySeenToAlreadyWrote(chosenOneVertexA, chosenOneVertexC,
                                                                 adjacencyListVertexB,
                                                                 adjacencyListAlreadyWroteVertexB);
            // De même pour C
            moveVertexFromAdjacencyListAlreadySeenToAlreadyWrote(chosenOneVertexA, chosenOneVertexB,
                                                                 adjacencyListVertexC,
                                                                 adjacencyListAlreadyWroteVertexC);


            // On laisse le traitement de fin d'algo décidé si on supprime les sommets de vertexInBorderBC, et autres traitements qui seraient influencé par une de ces listes vides
        }
        else {
            // Vérification de la cohérence des données, (c'est à dire que si B est marqué comme InBorderBC, il faut qu'il le soit vraiment et parreil pour C)
            bool testFindBInBorderBC = false;
            bool testFindCInBorderBC = false;
            for (int i = 0; i < meshData.vertexInBorderBC.size(); i++) {
                if (meshData.vertexInBorderBC[i] == chosenOneVertexB) {
                    testFindBInBorderBC = true;
                }
                if (meshData.vertexInBorderBC[i] == chosenOneVertexC) {
                    testFindCInBorderBC = true;
                }
            }
            if (!testFindBInBorderBC && BfindInBorderBC) {
                //std::cout << "Error: Vertex B is not in border BC but BfindInBorderBC is true." << std::endl;
            }
            else if (testFindBInBorderBC && !BfindInBorderBC) {
                //std::cout << "Error: Vertex B is in border BC but BfindInBorderBC is false." << std::endl;
            }
            if (!testFindCInBorderBC && CfindInBorderBC) {
                //std::cout << "Error: Vertex C is not in border BC but CfindInBorderBC is true." << std::endl;
            }
            else if (testFindCInBorderBC && !CfindInBorderBC) {
                //std::cout << "Error: Vertex C is in border BC but CfindInBorderBC is false." << std::endl;
            }

            // On va chercher un voisin de A qui est soit présent dans ces voisins déjà écrits, car s'il est dans ses voisins déjà écrits alors, il est dans vertexInBorderBC
            std::vector<int>& adjacencyListAlreadyWroteVertexA = meshData.adjacencyListAlreadyWrote[chosenOneVertexA];
            std::vector<int>& adjacencyListVertexA = meshData.adjacencyListAlreadySeen[chosenOneVertexA];

            // Si on a trouvé le voisin B via les voisins déjà écrits de A, alors pour trouver C,
            // il faut forcément chercher dans les voisins en commun de A et B qui sont hors de la border (meme principe vu qu'on doit forcément trouvé B dans la borderBC alors les autres boucles ne devraient pas servir)
            std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[chosenOneVertexB];
            std::vector<int>& adjacencyListAlreadyWroteVertexB = meshData.adjacencyListAlreadyWrote[chosenOneVertexB];
            std::vector<int> commonNeighbors;

            std::vector<int> adjacencyListAlreadyWroteVertexC = meshData.adjacencyListAlreadyWrote[chosenOneVertexC];
            std::vector<int>& adjacencyListVertexC = meshData.adjacencyListAlreadySeen[chosenOneVertexC];

            // On a nos trois sommets, on va rajouter les sommets qui n'ont pas été trouvé dans vertexInBorderBC
            if (!BfindInBorderBC && !CfindInBorderBC) {
                // TODO :
                //std::cout << "Vertex B is not in border BC and C is not in border BC " << std::endl;
                // On a nos deux sommets, on va les rajouter dans vertexInBorderBC
                meshData.vertexInBorderBC.push_back(chosenOneVertexB);
                meshData.vertexInBorderBC.push_back(chosenOneVertexC);
                // On va les supprimer de vertexNotInBorder car ils sont immuables maintenant, si ca pas déjà été fait
                auto deleteFromVertexNotInBorder = [&](int vertexId) {
                    for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
                        if (meshData.vertexNotInBorder[i].vertexId == vertexId) {
                            meshData.vertexNotInBorder.erase(meshData.vertexNotInBorder.begin() + i);
                            break;
                        }
                    }
                };
                deleteFromVertexNotInBorder(chosenOneVertexB);
                deleteFromVertexNotInBorder(chosenOneVertexC);

                // On va remplir les maps de chaque sommet concernant adjacencyListAlreadyWrote
                //Sommet A
                meshData.adjacencyListAlreadyWrote[chosenOneVertexA].push_back(chosenOneVertexB);
                meshData.adjacencyListAlreadyWrote[chosenOneVertexA].push_back(chosenOneVertexC);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexA]);
                // Sommet B
                meshData.adjacencyListAlreadyWrote[chosenOneVertexB].push_back(chosenOneVertexA);
                meshData.adjacencyListAlreadyWrote[chosenOneVertexB].push_back(chosenOneVertexC);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexB]);
                // Sommet C
                meshData.adjacencyListAlreadyWrote[chosenOneVertexC].push_back(chosenOneVertexA);
                meshData.adjacencyListAlreadyWrote[chosenOneVertexC].push_back(chosenOneVertexB);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexC]);

                // Lambda function to delete vertex from adjacency list
                auto deleteVertexFromAdjacencyList = [&](int vertex1, int vertex2,
                                                         std::vector<int>& adjacencyListAlreadySeen) {
                    for (int i = 0; i < adjacencyListAlreadySeen.size(); i++) {
                        int neighbor = adjacencyListAlreadySeen[i];
                        if (neighbor == vertex1 || neighbor == vertex2) {
                            adjacencyListAlreadySeen.erase(adjacencyListAlreadySeen.begin() + i);
                            i--;
                        }
                    }
                };

                // On va s'occuper de la mise à jour des listes d'adjacence
                // On va supprimer le sommet B et C de la liste d'adjacence de A
                deleteVertexFromAdjacencyList(chosenOneVertexB, chosenOneVertexC, adjacencyListVertexA);
                // On va supprimer le sommet A et C de la liste d'adjacence de B
                deleteVertexFromAdjacencyList(chosenOneVertexA, chosenOneVertexC, adjacencyListVertexB);
                // On va supprimer le sommet A et B de la liste d'adjacence de C
                deleteVertexFromAdjacencyList(chosenOneVertexA, chosenOneVertexB, adjacencyListVertexC);
            }
            else if (BfindInBorderBC && !CfindInBorderBC) {
                //std::cout << "Vertex B is in border BC and C is not in border BC " << std::endl;
                // On supprime le lien entre A et B de la liste d'adjacence déjà écrite de A et de B
                for (int i = 0; i < adjacencyListAlreadyWroteVertexA.size(); i++) {
                    if (adjacencyListAlreadyWroteVertexA[i] == chosenOneVertexB) {
                        adjacencyListAlreadyWroteVertexA.erase(adjacencyListAlreadyWroteVertexA.begin() + i);
                        break;
                    }
                }
                for (int i = 0; i < adjacencyListAlreadyWroteVertexB.size(); i++) {
                    if (adjacencyListAlreadyWroteVertexB[i] == chosenOneVertexA) {
                        adjacencyListAlreadyWroteVertexB.erase(adjacencyListAlreadyWroteVertexB.begin() + i);
                        break;
                    }
                }
                // Partie C
                meshData.vertexInBorderBC.push_back(chosenOneVertexC);
                // On va les supprimer de vertexNotInBorder car ils sont immuables maintenant, si ca pas déjà été fait
                for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
                    if (meshData.vertexNotInBorder[i].vertexId == chosenOneVertexC) {
                        meshData.vertexNotInBorder.erase(meshData.vertexNotInBorder.begin() + i);
                        break;
                    }
                }
                // On va remplir les maps de chaque sommet concernant adjacencyListAlreadyWrote
                //Sommet A
                meshData.adjacencyListAlreadyWrote[chosenOneVertexA].push_back(chosenOneVertexC);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexA]);
                // Sommet B
                meshData.adjacencyListAlreadyWrote[chosenOneVertexB].push_back(chosenOneVertexC);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexB]);
                // Sommet C
                meshData.adjacencyListAlreadyWrote[chosenOneVertexC].push_back(chosenOneVertexA);
                meshData.adjacencyListAlreadyWrote[chosenOneVertexC].push_back(chosenOneVertexB);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexC]);

                // On va s'occuper de la mise à jour des listes d'adjacence
                // On va supprimer le sommet C de la liste d'adjacence de A (car B est déjà supprimé)
                for (int i = 0; i < adjacencyListVertexA.size(); i++) {
                    int neighbor = adjacencyListVertexA[i];
                    if (neighbor == chosenOneVertexC) {
                        adjacencyListVertexA.erase(adjacencyListVertexA.begin() + i);
                        i--;
                    }
                }
                // On va supprimer le sommet C de la liste d'adjacence de B (car A est déjà supprimé)
                for (int i = 0; i < adjacencyListVertexB.size(); i++) {
                    int neighbor = adjacencyListVertexB[i];
                    if (neighbor == chosenOneVertexC) {
                        adjacencyListVertexB.erase(adjacencyListVertexB.begin() + i);
                        i--;
                    }
                }
                // Et pour le sommet C, on va supprimer A et B de sa liste d'adjacence déjà vue
                std::vector<int>& adjacencyListVertexC = meshData.adjacencyListAlreadySeen[chosenOneVertexC];
                for (int i = 0; i < adjacencyListVertexC.size(); i++) {
                    int neighbor = adjacencyListVertexC[i];
                    if (neighbor == chosenOneVertexA || neighbor == chosenOneVertexB) {
                        adjacencyListVertexC.erase(adjacencyListVertexC.begin() + i);
                        i--;
                    }
                }
            }
            else if (!BfindInBorderBC && CfindInBorderBC) {
                //std::cout << "Vertex C is in border BC and B is not in border BC " << std::endl;
                // On supprime le lien entre A et C de la liste d'adjacence déjà écrite de A et de C
                for (int i = 0; i < adjacencyListAlreadyWroteVertexA.size(); i++) {
                    if (adjacencyListAlreadyWroteVertexA[i] == chosenOneVertexC) {
                        adjacencyListAlreadyWroteVertexA.erase(adjacencyListAlreadyWroteVertexA.begin() + i);
                        break;
                    }
                }
                for (int i = 0; i < adjacencyListAlreadyWroteVertexC.size(); i++) {
                    if (adjacencyListAlreadyWroteVertexC[i] == chosenOneVertexA) {
                        adjacencyListAlreadyWroteVertexC.erase(adjacencyListAlreadyWroteVertexC.begin() + i);
                        break;
                    }
                }
                // Partie B
                meshData.vertexInBorderBC.push_back(chosenOneVertexB);
                // On va les supprimer de vertexNotInBorder car ils sont immuables maintenant, si ca pas déjà été fait
                for (int i = 0; i < meshData.vertexNotInBorder.size(); i++) {
                    if (meshData.vertexNotInBorder[i].vertexId == chosenOneVertexB) {
                        meshData.vertexNotInBorder.erase(meshData.vertexNotInBorder.begin() + i);
                        break;
                    }
                }
                // On va remplir les maps de chaque sommet concernant adjacencyListAlreadyWrote
                //Sommet A
                meshData.adjacencyListAlreadyWrote[chosenOneVertexA].push_back(chosenOneVertexB);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexA]);
                // Sommet C
                meshData.adjacencyListAlreadyWrote[chosenOneVertexC].push_back(chosenOneVertexB);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexC]);
                // Sommet B
                meshData.adjacencyListAlreadyWrote[chosenOneVertexB].push_back(chosenOneVertexA);
                meshData.adjacencyListAlreadyWrote[chosenOneVertexB].push_back(chosenOneVertexC);
                insertionSort(meshData.adjacencyListAlreadyWrote[chosenOneVertexB]);
                // On va s'occuper de la mise à jour des listes d'adjacence
                // On va supprimer le sommet B de la liste d'adjacence de A (car C est déjà supprimé)
                for (int i = 0; i < adjacencyListVertexA.size(); i++) {
                    int neighbor = adjacencyListVertexA[i];
                    if (neighbor == chosenOneVertexB) {
                        adjacencyListVertexA.erase(adjacencyListVertexA.begin() + i);
                        i--;
                    }
                }
                // On va supprimer le sommet B de la liste d'adjacence de C (car A est déjà supprimé)
                for (int i = 0; i < adjacencyListVertexC.size(); i++) {
                    int neighbor = adjacencyListVertexC[i];
                    if (neighbor == chosenOneVertexB) {
                        adjacencyListVertexC.erase(adjacencyListVertexC.begin() + i);
                        i--;
                    }
                }
                // Et pour le sommet B, on va supprimer A et C de sa liste d'adjacence déjà vue
                std::vector<int>& adjacencyListVertexB = meshData.adjacencyListAlreadySeen[chosenOneVertexB];
                for (int i = 0; i < adjacencyListVertexB.size(); i++) {
                    int neighbor = adjacencyListVertexB[i];
                    if (neighbor == chosenOneVertexA || neighbor == chosenOneVertexC) {
                        adjacencyListVertexB.erase(adjacencyListVertexB.begin() + i);
                        i--;
                    }
                }
            }
        }
        // std::cout << "Vertex A : " << chosenOneVertexA << std::endl;
        //std::cout << "Vertex B : " << chosenOneVertexB << "| Find in border BC : " << BfindInBorderBC << std::endl;
        //std::cout << "Vertex C : " << chosenOneVertexC << "| Find in border BC : " << CfindInBorderBC << std::endl;
    }

    deleteAndWriteThisTriangle(chosenOneVertexA, chosenOneVertexB, chosenOneVertexC, meshData, outputFile,
                               trianglesWritten);

    // la j'ecrit tout ce que qu'il y a dans le buffer pour le test de visualisation
    //outputFile.write(reinterpret_cast<char*>(inCoreTriangleBuffer.data()), numberToWrite * sizeof(TriangleCoordinates));

    //inCoreTriangleBuffer.clear();
    //*trianglesWritten += numberToWrite;

    return true;
}
