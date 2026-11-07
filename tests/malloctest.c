/*
 * malloctest: checks compat/src/gfamiga_malloc.c and times it against
 * libnix's malloc. The build makes two copies, malloctest (ours) and
 * malloctest-libnix; run both and compare the times.
 *   malloctest [blocks]      (default 200000)
 * Prints "FAIL ..." on any damaged block, otherwise "ok".
 *
 * Copyright (c) 2026 AmiCradle Team. MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static unsigned long rnd = 12345;
static unsigned next(void) { rnd = rnd * 1103515245UL + 12345UL; return (unsigned)(rnd >> 8); }

/* mostly small, like C++ nodes and strings, now and then big */
static size_t pick(void)
{
    unsigned r = next() % 1000;
    if ( r < 850 ) return 8 + next() % 120;
    if ( r < 990 ) return 128 + next() % 1900;
    if ( r < 999 ) return 2048 + next() % 30000;
    return 100000 + next() % 200000;
}

static int fails;

static void fill(unsigned char *p, size_t n, unsigned tag)
{
    size_t i;
    for ( i = 0; i < n; i++ ) p[i] = (unsigned char)(tag + i);
}

static void check(const unsigned char *p, size_t n, unsigned tag, const char *what)
{
    size_t i;
    for ( i = 0; i < n; i++ )
        if ( p[i] != (unsigned char)(tag + i) )
        {
            if ( fails++ < 5 )
                printf("FAIL %s: block %p byte %lu\n", what, (void *)p, (unsigned long)i);
            return;
        }
}

/* milliseconds, kept to integers (soft float has no printf %f to spare) */
static unsigned long ms(clock_t a, clock_t b) { return (unsigned long)(b - a) * 1000UL / CLOCKS_PER_SEC; }

int main(int argc, char **argv)
{
    unsigned long n = argc > 1 ? strtoul(argv[1], NULL, 10) : 200000UL;
    unsigned long live = n / 4, i, k;
    unsigned char **ptr = calloc(live, sizeof(*ptr));
    size_t *len = calloc(live, sizeof(*len));
    clock_t t0, t1, t2, t3;

    if ( !ptr || !len ) { printf("FAIL no table\n"); return 20; }
    printf("malloctest: %lu operations, %lu live slots\n", n, live);
    fflush(stdout);

    /* 1: fill every slot (the heap grows, as when a board loads) */
    t0 = clock();
    for ( i = 0; i < live; i++ )
    {
        len[i] = pick();
        ptr[i] = malloc(len[i]);
        if ( !ptr[i] ) { printf("FAIL malloc %lu\n", (unsigned long)len[i]); return 20; }
        fill(ptr[i], len[i] < 16 ? len[i] : 16, (unsigned)i);
    }
    t1 = clock();
    printf("grow: %lu blocks in %lu ms\n", live, ms(t0, t1));
    fflush(stdout);

    /* 2: churn: free, malloc and realloc at random */
    for ( k = 0; k < n; k++ )
    {
        i = next() % live;
        check(ptr[i], len[i] < 16 ? len[i] : 16, (unsigned)i, "churn");
        if ( next() % 4 == 0 )
        {
            size_t m = pick();
            unsigned char *q = realloc(ptr[i], m);
            if ( !q ) { printf("FAIL realloc\n"); return 20; }
            ptr[i] = q;
            if ( m < len[i] ) len[i] = m;
            check(ptr[i], len[i] < 16 ? len[i] : 16, (unsigned)i, "realloc");
            len[i] = m;
        }
        else
        {
            free(ptr[i]);
            len[i] = pick();
            ptr[i] = malloc(len[i]);
            if ( !ptr[i] ) { printf("FAIL malloc\n"); return 20; }
        }
        fill(ptr[i], len[i] < 16 ? len[i] : 16, (unsigned)i);
    }
    t2 = clock();
    printf("churn: %lu operations in %lu ms\n", n, ms(t1, t2));
    fflush(stdout);

    /* 3: free everything */
    for ( i = 0; i < live; i++ )
    {
        check(ptr[i], len[i] < 16 ? len[i] : 16, (unsigned)i, "free");
        free(ptr[i]);
    }
    t3 = clock();
    printf("free: %lu ms\ntotal: %lu ms\n%s\n", ms(t2, t3), ms(t0, t3), fails ? "FAILED" : "ok");
    free(ptr);
    free(len);
    return fails ? 10 : 0;
}
