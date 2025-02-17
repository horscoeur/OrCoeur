#ifndef UTILITY_H
#define UTILITY_H

#ifdef DEBUG
    #define DEBUG_LOG(msg) std::cout << msg << std::endl;
#else
    #define DEBUG_LOG(msg)
#endif

#include <cstdint>

// Utility function to swap the byte order (for big endian)
inline uint32_t swapUInt32(const uint32_t val) {
    return ((val & 0x000000FF) << 24) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0xFF000000) >> 24);
}

#endif //UTILITY_H
