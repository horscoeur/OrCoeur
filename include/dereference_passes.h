#ifndef DEREFERENCE_PASSES_H
#define DEREFERENCE_PASSES_H

#include <string>

/**
 * @brief Replaces the first vertex index (v1) with its corresponding vertex coordinate.
 *
 * This function processes a binary file containing triangle records where each record
 * holds three vertex indices (v1, v2, v3). It performs an external merge sort on the file
 * based on the first index (v1) and then synchronously scans both the sorted triangle file
 * and the vertex file using buffered I/O. When the vertex corresponding to v1 is found, a new
 * record (Triangle_Pass1) is created, where the index v1 is replaced by its actual coordinate.
 *
 * Buffered I/O is used here to read and write large blocks of records at once, thus reducing
 * the number of expensive disk operations.
 *
 * @param triangleIndexFile Path to the binary file containing TriangleIndices records.
 * @param vertexFile Path to the binary file containing Vertex records.
 * @param outputFile Path to the output binary file where Triangle_Pass1 records will be written.
 */
void dereferencePass1(const std::string &triangleIndexFile,
                      const std::string &vertexFile,
                      const std::string &outputFile);

/**
 * @brief Replaces the second vertex index (v2) with its corresponding vertex coordinate.
 *
 * This function takes as input a binary file containing Triangle_Pass1 records
 * (where v1 has already been dereferenced). It sorts these records by the second index (v2)
 * using an external merge sort. Then, using buffered I/O, it scans both the sorted file and the
 * vertex file concurrently. When a matching vertex for v2 is found, it replaces the index with the
 * vertex coordinate and writes the updated record (Triangle_Pass2) to the output.
 *
 * @param inputFile Path to the binary file containing Triangle_Pass1 records.
 * @param vertexFile Path to the binary file containing Vertex records.
 * @param outputFile Path to the output binary file where Triangle_Pass2 records will be written.
 */
void dereferencePass2(const std::string &inputFile,
                      const std::string &vertexFile,
                      const std::string &outputFile);

/**
 * @brief Replaces the third vertex index (v3) with its corresponding vertex coordinate.
 *
 * This function processes a binary file containing Triangle_Pass2 records (with v1 and v2 already
 * dereferenced). It sorts these records by the third index (v3) using an external merge sort.
 * Then, using buffered I/O for efficient reading and writing, it synchronously scans the sorted triangle
 * file and the vertex file. When the vertex for v3 is found, it replaces the index with the actual vertex
 * coordinate, outputting a complete TriangleCoordinates record.
 *
 * @param inputFile Path to the binary file containing Triangle_Pass2 records.
 * @param vertexFile Path to the binary file containing Vertex records.
 * @param outputFile Path to the output binary file where TriangleCoordinates records will be written.
 */
void dereferencePass3(const std::string &inputFile,
                      const std::string &vertexFile,
                      const std::string &outputFile);

                      
void externalMergeSortTrianglesIndices(const std::string &inputFile, const std::string &outputFile);
void externalMergeSortTrianglesPass1(const std::string &inputFile, const std::string &outputFile);
void externalMergeSortTrianglesPass2(const std::string &inputFile, const std::string &outputFile);



#endif //DEREFERENCE_PASSES_H
