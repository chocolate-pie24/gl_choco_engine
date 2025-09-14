#ifndef CHOCO_MACROS_H
#define CHOCO_MACROS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#ifdef __clang__
  #define NO_COVERAGE __attribute__((no_profile_instrument_function))
#else
  #define NO_COVERAGE
#endif

#define KIB ((size_t)(1ULL << 10))
#define MIB ((size_t)(1ULL << 20))
#define GIB ((size_t)(1ULL << 30))

#ifdef __cplusplus
}
#endif
#endif
