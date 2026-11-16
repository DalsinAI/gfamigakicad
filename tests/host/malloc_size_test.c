/*
 * Host test: the 32-bit overflow check and class index in
 * compat/include/gfamiga_malloc_size.h.
 *   cc -I compat/include tests/host/malloc_size_test.c -o malloc_size_test
 *   ./malloc_size_test
 *
 * Copyright (c) 2026 AmiCradle Team. MIT.
 */
#include <stdio.h>
#include <stdint.h>
#include "gfamiga_malloc_size.h"

static int fails;

static void check(const char *what, int ok)
{
    printf("%s %s\n", ok ? "ok  " : "FAIL", what);
    if ( !ok )
        fails++;
}

/* the old test, which wraps on a 32-bit size_t */
static int old_is_small(uint32_t n)
{
    return (uint32_t)(n + (uint32_t)GFAMIGA_HDR) <= (uint32_t)GFAMIGA_SMALL_MAX;
}

int main(void)
{
    uint32_t max_small = (uint32_t)(GFAMIGA_SMALL_MAX - GFAMIGA_HDR);
    unsigned ix;

    check("HDR is 8", GFAMIGA_HDR == 8);
    check("malloc(0) is small", gfamiga_malloc_is_small(0));
    check("malloc(1) is small", gfamiga_malloc_is_small(1));
    check("largest small request is small", gfamiga_malloc_is_small(max_small));
    check("one past the largest small request is large", !gfamiga_malloc_is_small(max_small + 1));

    ix = gfamiga_malloc_class_index(max_small);
    check("largest small index is in range", ix < GFAMIGA_CLASS_OF_LEN);

    ix = gfamiga_malloc_class_index(1);
    check("malloc(1) index is in range", ix < GFAMIGA_CLASS_OF_LEN);

    /* the bug: a huge n wrapped n+HDR into the small path, then the
     * class_of index was out of range (536870912 into a 257-entry table) */
    check("malloc((uint32_t)-1) is large", !gfamiga_malloc_is_small(0xFFFFFFFFu));
    check("malloc(0xFFFFFFF8) is large", !gfamiga_malloc_is_small(0xFFFFFFF8u));
    check("old test wrapped (uint32_t)-1 into small", old_is_small(0xFFFFFFFFu));
    check("old test wrapped 0xFFFFFFF8 into small", old_is_small(0xFFFFFFF8u));

    if ( fails )
    {
        printf("FAILED\n");
        return 1;
    }
    printf("all ok\n");
    return 0;
}
