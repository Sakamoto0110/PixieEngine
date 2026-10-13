#pragma once

#include <cstdio>

// A failed check prints where it is and is counted; main returns non-zero when any failed.
inline int pxCheckFailures = 0;

#define PX_CHECK(expr)                                                                   \
    do {                                                                                 \
        if (!(expr)) {                                                                   \
            ++pxCheckFailures;                                                           \
            std::printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr);         \
        }                                                                                \
    } while (0)
