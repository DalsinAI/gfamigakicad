/* Checks KiCad's zlib calls on z.library: crc32 of "123456789" must be
 * cbf43926, and a .gz file named on the command line is inflated as wx
 * does it (windowBits 15+32), printing each step's result.
 *   ztest [file.gz]
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "zlib.h"

int main( int argc, char** argv )
{
    static unsigned char in[65536], out[262144];
    z_stream s;
    FILE* f;
    unsigned long total = 0, steps = 0;
    int rc = Z_OK;
    time_t t0;

    printf( "crc32 %08lx (want cbf43926)\n",
            (unsigned long) crc32( 0, (const Bytef*) "123456789", 9 ) );
    printf( "adler32 %08lx (want 091e01de)\n",
            (unsigned long) adler32( 1, (const Bytef*) "123456789", 9 ) );

    if( argc < 2 )
        return 0;

    f = fopen( argv[1], "rb" );
    if( !f )
    {
        printf( "can't open %s\n", argv[1] );
        return 10;
    }

    memset( &s, 0, sizeof( s ) );
    rc = inflateInit2( &s, 15 + 32 );
    printf( "inflateInit2 %d\n", rc );
    t0 = time( NULL );

    while( rc == Z_OK )
    {
        if( s.avail_in == 0 )
        {
            s.avail_in = (uInt) fread( in, 1, sizeof( in ), f );
            s.next_in = in;
            if( s.avail_in == 0 )
                break;
        }

        s.next_out = out;
        s.avail_out = sizeof( out );
        rc = inflate( &s, Z_NO_FLUSH );
        total += sizeof( out ) - s.avail_out;

        if( ++steps <= 3 || rc != Z_OK )
            printf( "step %lu: rc %d, out %lu, in left %u\n", steps, rc, total, s.avail_in );

        if( steps > 100000 )
        {
            printf( "stopped: no end after %lu steps\n", steps );
            break;
        }
    }

    printf( "inflated %lu bytes in %lu steps, %ld s, rc %d\n", total, steps,
            (long) ( time( NULL ) - t0 ), rc );
    inflateEnd( &s );
    fclose( f );
    return rc == Z_STREAM_END ? 0 : 5;
}
