#include <arpa/inet.h>
#include <string.h>

#include "endian_utils.h"

static uint32_t swap_uint32(uint32_t value) {
    return ((value << 24) & 0xFF000000) |
           ((value << 8) & 0x00FF0000) |
           ((value >> 8) & 0x0000FF00) |
           ((value >> 24) & 0x000000FF);
}
static uint64_t swap_uint64(uint64_t value) {
    return ((value << 56) & 0xFF00000000000000ULL) |
           ((value << 40) & 0x00FF000000000000ULL) |
           ((value << 24) & 0x0000FF0000000000ULL) |
           ((value << 8)  & 0x000000FF00000000ULL) |
           ((value >> 8)  & 0x00000000FF000000ULL) |
           ((value >> 24) & 0x0000000000FF0000ULL) |
           ((value >> 40) & 0x000000000000FF00ULL) |
           ((value >> 56) & 0x00000000000000FFULL);
}

uint64_t htonll(uint64_t value) {
    return swap_uint64(value);
}
uint64_t ntohll(uint64_t value) {
    return swap_uint64(value);
}
uint32_t htonf(float value) {
    uint32_t swapped;
    memcpy(&swapped, &value, sizeof(swapped));
    return swap_uint32(swapped);
}
float ntohf(uint32_t value) {
    uint32_t swapped = swap_uint32(value);
    float result;
    memcpy(&result, &swapped, sizeof(result));
    return result;
}
uint64_t htond(double value) {
    uint64_t swapped;
    memcpy(&swapped, &value, sizeof(swapped));
    return swap_uint64(swapped);
}
double ntohd(uint64_t value) {
    uint64_t swapped = swap_uint64(value);
    double result;
    memcpy(&result, &swapped, sizeof(result));
    return result;
}