#ifndef ENDIAN_UTILS_H
#define ENDIAN_UTILS_H

#include <stdint.h>

uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);

uint32_t htonf(float value);
float ntohf(uint32_t value);

uint64_t htond(double value);
double ntohd(uint64_t value);

#endif // ENDIAN_UTILS_H