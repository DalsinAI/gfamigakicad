/*
 * gfamiga_malloc.c: a faster malloc, free and realloc for big programs.
 *
 * libnix's malloc keeps a list of 16 KB MemHeaders and calls exec's
 * Allocate() on each in turn until one has room, and free() walks the same
 * list to find the owner. KiCad makes millions of small allocations, the
 * list grows to thousands of headers and each call walks it (and each
 * header's own free list): loading a board spends most of its time there.
 *
 * Here, small requests (up to 2 KB) come from size classes: each class has
 * a free list, refilled from 64 KB arenas taken from exec in one go, so
 * malloc and free are a handful of instructions. Larger requests go straight
 * to AllocMem with a small header. A 4-byte word in front of every block
 * says which it is, so free() never searches. Everything is handed back to
 * exec at exit (AmigaOS does not do that for us).
 *
 * Small blocks freed go back on their class's list for reuse, not to exec,
 * so a program's peak small-object memory stays held until it quits.
 *
 * Replaces libnix's malloc.o (malloc, free, __initmalloc, __exitmalloc) and
 * realloc.o, which reads malloc's header. calloc, strdup and C++ new call
 * malloc and need nothing. Link it as an object, ahead of libnix.
 *
 * Copyright (c) 2026 AmiCradle Team. MIT.
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <proto/exec.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

extern struct ExecBase *SysBase;

#define ARENA_SIZE  (64 * 1024)
#define SMALL_MAX   2048            /* largest chunk (header included) */
#define HDR         4               /* the word in front of each block */
#define LARGE_HDR   16              /* MinNode, pad, then the word */

/* chunk sizes, header included, multiples of 8: fine steps where most
 * requests are (C++ nodes, strings), coarser further up */
static const UWORD class_size[] = {
      8,   16,   24,   32,   40,   48,   56,   64,   72,   80,   88,   96,
    104,  112,  120,  128,  160,  192,  224,  256,  320,  384,  448,  512,
    640,  768,  896, 1024, 1280, 1536, 1792, 2048
};
#define NCLASS (sizeof(class_size) / sizeof(class_size[0]))

/* request size (header included) / 8 -> class, for sizes up to SMALL_MAX */
static UBYTE class_of[SMALL_MAX / 8 + 1];

struct FreeChunk { struct FreeChunk *next; };

static struct SignalSemaphore lock;
static struct FreeChunk      *free_list[NCLASS];
static UBYTE                 *bump, *bump_end;   /* the arena being carved */
static struct MinList         arenas;            /* every arena, for exit */
static struct MinList         large;             /* every large block */
static int                    ready;

static void newlist(struct MinList *l)
{
    l->mlh_Head = (struct MinNode *)&l->mlh_Tail;
    l->mlh_Tail = NULL;
    l->mlh_TailPred = (struct MinNode *)&l->mlh_Head;
}

void __initmalloc(void)
{
    unsigned c = 0, s;

    for ( s = 0; s <= SMALL_MAX / 8; s++ )
    {
        while ( class_size[c] < s * 8 )
            c++;
        class_of[s] = (UBYTE)c;
    }
    InitSemaphore(&lock);
    newlist(&arenas);
    newlist(&large);
    ready = 1;
}

void __exitmalloc(void)
{
    struct MinNode *n;

    if ( !ready )
        return;
    while ( (n = (struct MinNode *)RemHead((struct List *)&large)) != NULL )
        FreeMem(n, ((ULONG *)n)[3]);
    while ( (n = (struct MinNode *)RemHead((struct List *)&arenas)) != NULL )
        FreeMem(n, ARENA_SIZE);
    ready = 0;
}

/* the init and exit lists libnix's startup runs, at libnix malloc's place */
__asm__(".section .list___INIT_LIST__,\"aw\"\n\t.long ___initmalloc\n\t.long 78\n\t.text");
__asm__(".section .list___EXIT_LIST__,\"aw\"\n\t.long ___exitmalloc\n\t.long 78\n\t.text");

static void *small_alloc(unsigned c)
{
    struct FreeChunk *f = free_list[c];
    ULONG sz;
    UBYTE *p;

    if ( f )
    {
        free_list[c] = f->next;
        p = (UBYTE *)f;
    }
    else
    {
        sz = class_size[c];
        if ( bump + sz > bump_end )
        {
            /* a new arena; the old one's tail (under 2 KB) is left unused */
            UBYTE *a = AllocMem(ARENA_SIZE, MEMF_ANY);
            if ( !a )
                return NULL;
            AddTail((struct List *)&arenas, (struct Node *)a);
            bump = a + 8;          /* after its MinNode, 8-aligned */
            bump_end = a + ARENA_SIZE;
        }
        p = bump;
        bump += sz;
    }
    *(ULONG *)p = c;
    return p + HDR;
}

static void *large_alloc(size_t n)
{
    ULONG total = (n + LARGE_HDR + 7) & ~7UL;
    UBYTE *b;

    if ( total < n )
        return NULL;
    b = AllocMem(total, MEMF_ANY);
    if ( !b )
        return NULL;
    AddTail((struct List *)&large, (struct Node *)b);
    ((ULONG *)b)[3] = total;      /* the word in front of the data */
    return b + LARGE_HDR;
}

void *malloc(size_t n)
{
    void *p;

    if ( !ready )
        return NULL;
    ObtainSemaphore(&lock);
    if ( n + HDR <= SMALL_MAX )
        p = small_alloc(class_of[(n + HDR + 7) >> 3]);
    else
        p = large_alloc(n);
    ReleaseSemaphore(&lock);
    if ( !p )
        errno = ENOMEM;
    return p;
}

/* bytes the block can hold */
static size_t usable(void *p)
{
    ULONG w = ((ULONG *)p)[-1];

    return w < NCLASS ? (size_t)class_size[w] - HDR : (size_t)w - LARGE_HDR;
}

void free(void *p)
{
    ULONG w;

    if ( !p )
        return;
    w = ((ULONG *)p)[-1];
    ObtainSemaphore(&lock);
    if ( w < NCLASS )
    {
        struct FreeChunk *f = (struct FreeChunk *)((UBYTE *)p - HDR);
        f->next = free_list[w];
        free_list[w] = f;
    }
    else
    {
        struct MinNode *b = (struct MinNode *)((UBYTE *)p - LARGE_HDR);
        Remove((struct Node *)b);
        FreeMem(b, w);
    }
    ReleaseSemaphore(&lock);
}

void *realloc(void *p, size_t n)
{
    size_t have;
    void *q;

    if ( !p )
        return malloc(n);
    if ( !n )
    {
        free(p);
        return NULL;
    }
    have = usable(p);
    /* stays put when it fits and isn't wasting most of a large block */
    if ( n <= have && (have <= SMALL_MAX || n > have / 2) )
        return p;
    q = malloc(n);
    if ( q )
    {
        memcpy(q, p, n < have ? n : have);
        free(p);
    }
    return q;
}
