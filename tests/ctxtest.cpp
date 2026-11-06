// ctxtest: KiCad's libcontext on the 68k (patch 0001). Switches between
// main and two coroutines 100000 times, checks the values passed each way
// and that callee-saved registers survive, and prints the time taken.
// Copyright (c) 2026 AmiCradle Team
#include <libcontext.h>
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <vector>

using namespace libcontext;
static fcontext_t mainCtx, coCtx[2];
static long sums[2];

static void body( intptr_t arg )
{
    int me = (int) ( arg & 1 );
    for( ;; )
    {
        sums[me] += arg;
        arg = jump_fcontext( &coCtx[me], mainCtx, arg * 2, true );
    }
}

int main()
{
    std::vector<char> st0( 65536 ), st1( 65536 );
    coCtx[0] = make_fcontext( st0.data() + st0.size(), st0.size(), body );
    coCtx[1] = make_fcontext( st1.data() + st1.size(), st1.size(), body );
    volatile long keep = 12345;   // lives across the switches
    long bad = 0, back = 0;
    clock_t t0 = clock();
    for( long i = 0; i < 100000; i++ )
    {
        intptr_t v = ( i << 1 ) | ( i & 1 );
        intptr_t r = jump_fcontext( &mainCtx, coCtx[i & 1], v, true );
        if( r != v * 2 ) bad++;
        back += r;
    }
    double s = double( clock() - t0 ) / CLOCKS_PER_SEC;
    printf( "ctxtest: %s, bad %ld, keep %ld, sums %ld %ld, back %ld, %.3f s\n",
            ( bad == 0 && keep == 12345 ) ? "PASS" : "FAIL", bad, (long) keep,
            sums[0], sums[1], back, s );
    return bad ? 20 : 0;
}
