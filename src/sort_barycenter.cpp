#include "structures.h"
#include "mesh_conversion.h"
#include "sort_barycenter.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstring>
#include <filesystem>
#include <queue>
#include <string>

Vertex calculateBarycenter(const TriangleCoordinates &triangle)
{
    Vertex barycenter;
    barycenter.x = (triangle.v1.x + triangle.v2.x + triangle.v3.x) / 3.0f;
    barycenter.y = (triangle.v1.y + triangle.v2.y + triangle.v3.y) / 3.0f;
    barycenter.z = (triangle.v1.z + triangle.v2.z + triangle.v3.z) / 3.0f;
    return barycenter;
}

bool ChunkInfo::operator>(const ChunkInfo &other) const
{
    switch (sortAxis)
    {
    case 'x':
        return triangleWithBarycenter.barycenter.x > other.triangleWithBarycenter.barycenter.x;
    case 'y':
        return triangleWithBarycenter.barycenter.y > other.triangleWithBarycenter.barycenter.y;
    case 'z':
        return triangleWithBarycenter.barycenter.z > other.triangleWithBarycenter.barycenter.z;
    default:
        return triangleWithBarycenter.barycenter.x > other.triangleWithBarycenter.barycenter.x;
    }
}

void sortChunk(std::vector<TriangleWithBarycenter> &triangles, char sortAxis)
{
    std::sort(triangles.begin(), triangles.end(),
              [sortAxis](const TriangleWithBarycenter &a, const TriangleWithBarycenter &b)
              {
                  switch (sortAxis)
                  {
                  case 'x':
                      return a.barycenter.x < b.barycenter.x;
                  case 'y':
                      return a.barycenter.y < b.barycenter.y;
                  case 'z':
                      return a.barycenter.z < b.barycenter.z;
                  default:
                      return a.barycenter.x < b.barycenter.x;
                  }
              });
}

bool sortTrianglesByBarycenter(const std::string &inputFilename, const std::string &outputFilename, char sortAxis)
{
    std::cout << "Sorting triangles by " << sortAxis << "-axis barycenter..." << std::endl;


    std::ifstream inputFile(inputFilename, std::ios::binary);
    if (!inputFile)
    {
        std::cerr << "Error: Unable to open " << inputFilename << std::endl;
        return false;
    }

    auto [format, faceCount] = parseORCOEURHeader(inputFile);

    std::vector<std::string> runFiles;
    int runCount = 0;

    TriangleCoordinates triangle;
    int chunkCount = 0;
    std::vector<TriangleWithBarycenter> buffer;
    buffer.reserve(CHUNK_SIZE);

    while (inputFile.read(reinterpret_cast<char *>(&triangle), sizeof(TriangleCoordinates))){
        TriangleWithBarycenter t;
        t.triangle = triangle;
        t.barycenter = calculateBarycenter(triangle);
        buffer.push_back(t);

        if (buffer.size() >= CHUNK_SIZE){
            // Create chunk file
            std::string tempFilename = "temp_chunk_" + std::to_string(chunkCount) + ".bin";

            // Sorts the buffer
            sortChunk(buffer, sortAxis);

            // Write to the chunk file
            std::ofstream tempFile(tempFilename, std::ios::binary);
            for (const auto &twb : buffer){
                tempFile.write(reinterpret_cast<const char *>(&twb), sizeof(TriangleWithBarycenter));
            }
            tempFile.close();

            runFiles.push_back(tempFilename);
            buffer.clear();
            chunkCount++;
        }
    }

    // If there are triangles inside the last chunk
    if (!buffer.empty())
    {
        std::string tempFilename = "temp_chunk_" + std::to_string(chunkCount) + ".bin";

        sortChunk(buffer, sortAxis);

        std::ofstream tempFile(tempFilename, std::ios::binary);
        for (const auto &triangleWithBarycenter : buffer)
        {
            tempFile.write(reinterpret_cast<const char *>(&triangleWithBarycenter), sizeof(TriangleWithBarycenter));
        }
        tempFile.close();

        runFiles.push_back(tempFilename);
        buffer.clear();
    }

    // If we have only one chunk
    if (runFiles.size() == 1){
        std::ifstream chunk(runFiles[0], std::ios::binary);
        std::ofstream outputFile(outputFilename, std::ios::binary);

        outputFile << "format: binary_little_endian\n";
        outputFile << "face_count: " << faceCount << "\n";
        outputFile << "END_HEADER\n";

        // Write only triangles without the barycenter
        TriangleWithBarycenter twb;
        while (chunk.read(reinterpret_cast<char *>(&twb), sizeof(TriangleWithBarycenter)))
        {
            outputFile.write(reinterpret_cast<const char *>(&twb.triangle), sizeof(TriangleCoordinates));
        }

        chunk.close();
        outputFile.close();

        // Delete the chunk file
        std::remove(runFiles[0].c_str());

        return true;
    }else{
        std::vector<std::ifstream> chunkFiles(runFiles.size());
        std::priority_queue<ChunkInfo, std::vector<ChunkInfo>, std::greater<ChunkInfo>> minHeap;

        for (int i = 0; i < chunkCount; i++){
            // Open the file
            chunkFiles[i].open(runFiles[i], std::ios::binary);
            if (!chunkFiles[i].is_open()){
                std::cerr << "Error opening chunk file " << runFiles[i] << std::endl;
                continue;
            }

            ChunkInfo info;
            info.file = &chunkFiles[i];

            if (chunkFiles[i].read(reinterpret_cast<char *>(&info.triangleWithBarycenter), sizeof(TriangleWithBarycenter))){
                minHeap.push(info);
            }
            else{
                std::cerr << "Error when reading the first triangle in " << runFiles[i] << std::endl;
                continue;
            }
        }

        std::ofstream outputFile(outputFilename, std::ios::binary);

        // Write header
        outputFile << "format: binary_little_endian\n";
        outputFile << "face_count: " << faceCount << "\n";
        outputFile << "END_HEADER\n";

        while (!minHeap.empty()){
            ChunkInfo top = minHeap.top();
            minHeap.pop();

            outputFile.write(reinterpret_cast<const char *>(&top.triangleWithBarycenter.triangle), sizeof(TriangleCoordinates));

            if (top.file->read(reinterpret_cast<char *>(&top.triangleWithBarycenter), sizeof(TriangleWithBarycenter))){
                minHeap.push(top);
            }
        }
        outputFile.close();
        for (auto &file : chunkFiles){
            if (file.is_open()){
                file.close();
            }
        }
        for (const auto &filename : runFiles){
            std::remove(filename.c_str());
        }
        return true;
    }
}