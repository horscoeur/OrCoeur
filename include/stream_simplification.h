#ifndef STREAM_SIMPLIFICATION_H
#define STREAM_SIMPLIFICATION_H

#include "structures.h"

bool streamSimplificationVisualization(const std::string &inputFile, const std::string &outputFile, int maxTrianglesInBuffer, float decimationPercentage, bool visualizeSimplification);

bool read(std::ifstream & inputFile, std::vector<TriangleCoordinates> & inCoreTriangleBuffer, int numberToRead, int * trianglesInCore);

bool decimate();

bool write(std::ofstream &outputFile, std::vector<TriangleCoordinates> & trianglesInCoreBuffer, int numberToWrite, int *trianglesWritten, int *trianglesInCore);

bool initBuffer(std::ifstream & inputFile, std::vector<TriangleCoordinates> & inCoreTriangleBuffer, int maxTrianglesInBuffer,float decimationPercentage, int * trianglesInCore);

void displayFromBuffer(std::vector<TriangleCoordinates> & inCoreTriangleBuffer, int nbTrianglesInCore);



#endif