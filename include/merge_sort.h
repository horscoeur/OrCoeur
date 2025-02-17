#ifndef MERGE_SORT_H
#define MERGE_SORT_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Maximum number of triangles loaded in memory to sort a run. (1 triangle = 12 bytes)
#define CHUNK_SIZE 1000000 // 12 MB

/**
 * @brief Represents a node in the heap used for k-way merge.
 *
 * @tparam T The type of elements to be sorted.
 */
template<typename T>
struct HeapNode {
    T record;
    int runIndex; // index of the run in runFiles
};

/**
 * @brief Performs an external merge sort on a file.
 *
 * This function divides the input data into chunks, sorts each chunk in memory,
 * writes the sorted chunks into temporary files ("runs"), and finally merges
 * these sorted chunks using a heap to produce the final sorted output.
 *
 * @tparam T The type of elements to be sorted.
 * @tparam Comparator The type of the comparator function used to compare the elements.
 * @param inputFile The path to the input file containing the unsorted data.
 * @param outputFile The path to the output file where the sorted data will be written.
 * @param comp The comparator function to compare elements of type T.
 */
template<typename T, typename Comparator>
void externalMergeSort(const std::string &inputFile, const std::string &outputFile, Comparator comp) {

    // Phase 1: Create sorted runs
    std::ifstream in(inputFile, std::ios::binary);
    if (!in) {
        std::cerr << "Error: Unable to open " << inputFile << std::endl;
        return;
    }
    std::vector<std::string> runFiles;
    int runCount = 0;

    // Until the end of the file is reached, read a chunk, sort it and write it to a run file
    while (true) {
        std::vector<T> buffer;
        buffer.resize(CHUNK_SIZE);
        int count = 0;

        // Read a chunk from the input file into the buffer
        while (count < CHUNK_SIZE && in.read(reinterpret_cast<char *>(&buffer[count]), sizeof(T))) {
            count++;
        }

        // If no data is read, break out of the loop
        if (count == 0) break;
        buffer.resize(count);

        // Sort the chunk in memory using the provided comparator
        std::sort(buffer.begin(), buffer.end(), comp);

        // Create a temporary file to store this sorted chunk ("run")
        std::string runFileName = "run_" + std::to_string(runCount) + ".bin";
        std::ofstream runFile(runFileName, std::ios::binary);
        if (!runFile) {
            std::cerr << "Error: Unable to write " << runFileName << std::endl;
            return;
        }

        // Write the sorted chunk to the run file
        runFile.write(reinterpret_cast<char *>(buffer.data()), count * sizeof(T));
        runFile.close();
        runFiles.push_back(runFileName);
        runCount++;

        // If we reached the end of the file, stop reading
        if (in.eof()) break;
    }
    in.close();

    // Phase 2: k-way merge using a heap
    // Comparator for the heap (min-heap) to always pop the smallest element
    auto heapComparator = [comp](const HeapNode<T> &a, const HeapNode<T> &b) {
        return comp(b.record, a.record); // for a min-heap
    };
    std::vector<HeapNode<T>> heap;

    // Open all run files for reading
    std::vector<std::ifstream *> runStreams;
    for (size_t i = 0; i < runFiles.size(); i++) {
        auto *stream = new std::ifstream(runFiles[i], std::ios::binary);
        if (!stream->is_open()) {
            std::cerr << "Error: Unable to open " << runFiles[i] << std::endl;
            return;
        }
        runStreams.push_back(stream);

        // Read the first element from each run file and insert it into the heap
        T rec{};
        if (stream->read(reinterpret_cast<char *>(&rec), sizeof(T))) {
            heap.push_back({rec, static_cast<int>(i)});
        }
    }

    // Open the output file to write the sorted data
    std::ofstream out(outputFile, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Unable to open " << outputFile << std::endl;
        return;
    }

    // Perform the k-way merge: Extract the smallest element, write it to the output file,
    // and insert the next element from the corresponding run file into the heap.
    while (!heap.empty()) {
        // Pop the smallest element from the heap
        std::pop_heap(heap.begin(), heap.end(), heapComparator);
        HeapNode node = heap.back();
        heap.pop_back();

        // Write the smallest element to the output file
        out.write(reinterpret_cast<char*>(&node.record), sizeof(T));

        // Read the next element from the run file and push it to the heap
        int idx = node.runIndex;
        T rec{};
        if (runStreams[idx]->read(reinterpret_cast<char*>(&rec), sizeof(T))) {
            heap.push_back({rec, idx});
            std::push_heap(heap.begin(), heap.end(), heapComparator);
        }
    }
    out.close();

    // Close and clean up streams and delete temporary files
    for (size_t i = 0; i < runStreams.size(); i++) {
        runStreams[i]->close();
        delete runStreams[i];
        std::remove(runFiles[i].c_str()); // Remove temporary run files
    }
}

#endif //MERGE_SORT_H
