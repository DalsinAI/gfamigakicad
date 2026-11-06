/* The toolkit's wxWidgets was built with its own zlib under the wx_zlib_
 * prefix; KiCad links no such zlib, so its names lead to z.library's calls
 * (gfamiga_zlib.c) here.
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#include "zlib.h"

const char* wx_zlib_zlibVersion( void ) { return zlibVersion(); }
int wx_zlib_deflateInit_( z_streamp s, int l, const char* v, int n ) { return deflateInit_( s, l, v, n ); }
int wx_zlib_deflateInit2_( z_streamp s, int l, int m, int w, int ml, int st, const char* v, int n )
{
    return deflateInit2_( s, l, m, w, ml, st, v, n );
}
int wx_zlib_deflate( z_streamp s, int f ) { return deflate( s, f ); }
int wx_zlib_deflateEnd( z_streamp s ) { return deflateEnd( s ); }
int wx_zlib_deflateReset( z_streamp s ) { return deflateReset( s ); }
int wx_zlib_deflateSetDictionary( z_streamp s, const Bytef* d, uInt n ) { return deflateSetDictionary( s, d, n ); }
int wx_zlib_inflateInit_( z_streamp s, const char* v, int n ) { return inflateInit_( s, v, n ); }
int wx_zlib_inflateInit2_( z_streamp s, int w, const char* v, int n ) { return inflateInit2_( s, w, v, n ); }
int wx_zlib_inflate( z_streamp s, int f ) { return inflate( s, f ); }
int wx_zlib_inflateEnd( z_streamp s ) { return inflateEnd( s ); }
int wx_zlib_inflateReset( z_streamp s ) { return inflateReset( s ); }
int wx_zlib_inflateReset2( z_streamp s, int w ) { return inflateReset2( s, w ); }
int wx_zlib_inflateSetDictionary( z_streamp s, const Bytef* d, uInt n ) { return inflateSetDictionary( s, d, n ); }
int wx_zlib_inflateValidate( z_streamp s, int c ) { return inflateValidate( s, c ); }
uLong wx_zlib_crc32( uLong c, const Bytef* b, uInt n ) { return crc32( c, b, n ); }
uLong wx_zlib_adler32( uLong a, const Bytef* b, uInt n ) { return adler32( a, b, n ); }
