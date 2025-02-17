#ifndef CONVERSION_H
#define CONVERSION_H

#include <string>

#include "mesh_conversion.h"
#include "structures.h"

/**
 * @brief Exports a binary OBJSoup file to a well-formatted text file.
 *
 * This function reads a binary OBJSoup file and writes its contents to a text file
 *
 * @param inputFilename Input binary OBJSoup file path.
 * @param outputFilename Output text file path.
 * @param faceCount Number of faces in the OBJSoup file.
 */
void exportBinaryOBJSoupToText(const std::string &inputFilename, const std::string &outputFilename, int faceCount);

// ---------------- Conversion Functions ----------------

/**
 * @brief Converts an OBJ file to an OBJSoup format.
 *
 * The function reads the OBJ file, writes the vertices to a binary file and
 * the triangle indices to another binary file. It then performs external sorting
 * on the triangles and dereferences the indices to produce the final output.
 * If the output file ends with ".bin", the binary file is kept, otherwise it is converted to text. (costly)
 *
 * @param objFilename Input OBJ file path.
 * @param outputFilename Output OBJSoup file path.
 */
void convertOBJtoOBJSoup(const std::string &objFilename, const std::string &outputFilename);

/**
 * @brief Converts a PLY file to an OBJSoup format.
 *
 * The function reads the PLY file, writes the vertices to a binary file and
 * the triangle indices to another binary file. It then performs external sorting
 * on the triangles and dereferences the indices to produce the final output.
 * If the output file ends with ".bin", the binary file is kept, otherwise it is converted to text. (costly)
 *
 * @param plyFilename Input PLY file path.
 * @param outputFilename Output OBJSoup file path.
 */
void convertPLYtoOBJSoup(const std::string &plyFilename, const std::string &outputFilename);

/**
 * @brief Parses the header of a PLY file to extract the number of vertices and faces.
 *
 * This function reads the header of a PLY file and extracts the number of vertices and faces
 * as well as the format of the file (ascii, binary_little_endian or binary_big_endian).
 *
 * @param in Input stream of the PLY file.
 * @param header PLYHeader structure to store the extracted information.
 * @return True if the header was successfully parsed, false otherwise.
 */
bool parsePLYHeader(std::istream &in, PLYHeader &header);

/**
 * @brief Processes a PLY file and writes the vertices and faces to temporary binary files.
 *
 * This function reads a PLY file and writes the vertex coordinates to a binary file and the
 * triangle indices to another binary file. The binary files are then used for further processing.
 *
 * @param plyFilename Input PLY file path.
 * @param tempVertexFile Path for a temporary file for vertices.
 * @param tempTriangleIndicesFile Path for a temporary file for triangle indices.
 * @return The number of faces in the PLY file.
 */
int processPLYFile(const std::string &plyFilename, const std::string &tempVertexFile,
                   const std::string &tempTriangleIndicesFile);

/**
 * @brief Converts an OBJSoup file back to an OBJ file.
 *
 * This function converts an OBJSoup file (binary or text)back into an OBJ file.
 * The processing is performed in an out-of-core fashion.
 *
 * @param inputFilename Input OBJSoup file path.
 * @param outputFilename Output OBJ file path.
 */
void convertOBJSoupToOBJ(const std::string &inputFilename, const std::string &outputFilename);

/**
 * @brief Processes a binary file and writes the vertices and faces to text files.
 *
 * This function reads a binary file containing triangle coordinates and writes the vertex coordinates
 * and face indices to separate text files.
 *
 * @param file Input binary file stream.
 * @param vertexFile Output text file stream for vertices.
 * @param faceFile Output text file stream for faces.
 */
void processBinary(std::ifstream &file, std::ofstream &vertexFile, std::ofstream &faceFile);

/**
 * @brief Processes an ASCII file and writes the vertices and faces to text files.
 *
 * This function reads an ASCII file containing triangle coordinates and writes the vertex coordinates
 * and face indices to separate text files.
 *
 * @param file Input ASCII file stream.
 * @param vertexFile Output text file stream for vertices.
 * @param faceFile Output text file stream for faces.
 */
void processASCII(std::ifstream &file, std::ofstream &vertexFile, std::ofstream &faceFile);

#endif // CONVERSION_H