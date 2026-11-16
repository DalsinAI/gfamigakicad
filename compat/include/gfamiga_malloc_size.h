/*
 * Size-class decision for gfamiga_malloc.c, in plain C so the host
 * test (tests/host/malloc_size_test.c) can check the 32-bit overflow
 * case without Exec.
 *
 * Copyright (c) 2026 AmiCradle Team. MIT.
 */
#ifndef GFAMIGA_MALLOC_SIZE_H
#define GFAMIGA_MALLOC_SIZE_H

#include <stdint.h>

#define GFAMIGA_SMALL_MAX 2048 /* largest chunk, header included */
#define GFAMIGA_HDR       8    /* word in front of the block, padded to 8 so
                                * the user pointer is 8-aligned */

#define GFAMIGA_CLASS_OF_LEN (GFAMIGA_SMALL_MAX / 8 + 1)

/* 1 if a request of n bytes is a small-class allocation, on a 32-bit
 * size_t (the Amiga). A huge n must not wrap into the small path. */
static inline int gfamiga_malloc_is_small(uint32_t n)
{
    return n <= (uint32_t)(GFAMIGA_SMALL_MAX - GFAMIGA_HDR);
}

/* class_of index for a small request: (n + HDR + 7) / 8. The table has
 * GFAMIGA_CLASS_OF_LEN entries (0..256). Only call when is_small. */
static inline unsigned gfamiga_malloc_class_index(uint32_t n)
{
    return (unsigned)((n + (uint32_t)GFAMIGA_HDR + 7u) >> 3);
}

#endif
