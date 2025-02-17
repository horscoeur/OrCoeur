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

// Utility function to swap the byte order (for big endian)
inline float swapFloat(float val) {
    uint32_t temp = *reinterpret_cast<uint32_t*>(&val);
    temp = ((temp >> 24) & 0x000000FF) |
           ((temp >>  8) & 0x0000FF00) |
           ((temp <<  8) & 0x00FF0000) |
           ((temp << 24) & 0xFF000000);
    return *reinterpret_cast<float*>(&temp);
}


#endif //UTILITY_H
