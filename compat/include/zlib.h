/* zlib.h's API, answered by the system's z.library (GFOpenZlib in
 * amicradle-guest libraries/openzlib, LVOs from its z.lvo): KiCad, libgit2
 * and the toolkit's streams link no zlib of their own (gfamiga_zlib.c).
 * z_stream and gz_header are laid out as zlib.h lays them out on the 68k.
 * Not offered: the gz* file calls, inflateBack.
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#ifndef ZLIB_H
#define ZLIB_H

#include "zconf.h"

#define ZLIB_VERSION "1.3.1"
#define ZLIB_VERNUM 0x1310
#define ZLIB_VER_MAJOR 1
#define ZLIB_VER_MINOR 3
#define ZLIB_VER_REVISION 1
#define ZLIB_VER_SUBREVISION 0

#ifdef __cplusplus
extern "C" {
#endif

typedef voidpf (*alloc_func)( voidpf opaque, uInt items, uInt size );
typedef void   (*free_func)( voidpf opaque, voidpf address );

struct internal_state;

typedef struct z_stream_s
{
    z_const Bytef* next_in;
    uInt         avail_in;
    uLong        total_in;
    Bytef*       next_out;
    uInt         avail_out;
    uLong        total_out;
    char*        msg;
    struct internal_state FAR* state;
    alloc_func   zalloc;
    free_func    zfree;
    voidpf       opaque;
    int          data_type;
    uLong        adler;
    uLong        reserved;
} z_stream;

typedef z_stream FAR* z_streamp;

typedef struct gz_header_s
{
    int    text;
    uLong  time;
    int    xflags;
    int    os;
    Bytef* extra;
    uInt   extra_len;
    uInt   extra_max;
    Bytef* name;
    uInt   name_max;
    Bytef* comment;
    uInt   comm_max;
    int    hcrc;
    int    done;
} gz_header;

typedef gz_header FAR* gz_headerp;

#define Z_NO_FLUSH      0
#define Z_PARTIAL_FLUSH 1
#define Z_SYNC_FLUSH    2
#define Z_FULL_FLUSH    3
#define Z_FINISH        4
#define Z_BLOCK         5
#define Z_TREES         6

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

#define Z_NO_COMPRESSION         0
#define Z_BEST_SPEED             1
#define Z_BEST_COMPRESSION       9
#define Z_DEFAULT_COMPRESSION  (-1)

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_RLE                 3
#define Z_FIXED               4
#define Z_DEFAULT_STRATEGY    0

#define Z_BINARY   0
#define Z_TEXT     1
#define Z_ASCII    Z_TEXT
#define Z_UNKNOWN  2

#define Z_DEFLATED   8
#define Z_NULL  0

#define zlib_version zlibVersion()

ZEXTERN const char* ZEXPORT zlibVersion( void );
ZEXTERN uLong ZEXPORT zlibCompileFlags( void );

ZEXTERN int ZEXPORT deflateInit_( z_streamp strm, int level, const char* version, int stream_size );
ZEXTERN int ZEXPORT deflateInit2_( z_streamp strm, int level, int method, int windowBits,
                                   int memLevel, int strategy, const char* version,
                                   int stream_size );
ZEXTERN int ZEXPORT deflate( z_streamp strm, int flush );
ZEXTERN int ZEXPORT deflateEnd( z_streamp strm );
ZEXTERN int ZEXPORT deflateSetDictionary( z_streamp strm, const Bytef* dictionary, uInt dictLength );
ZEXTERN int ZEXPORT deflateGetDictionary( z_streamp strm, Bytef* dictionary, uInt* dictLength );
ZEXTERN int ZEXPORT deflateCopy( z_streamp dest, z_streamp source );
ZEXTERN int ZEXPORT deflateReset( z_streamp strm );
ZEXTERN int ZEXPORT deflateParams( z_streamp strm, int level, int strategy );
ZEXTERN int ZEXPORT deflateTune( z_streamp strm, int good_length, int max_lazy, int nice_length,
                                 int max_chain );
ZEXTERN uLong ZEXPORT deflateBound( z_streamp strm, uLong sourceLen );
ZEXTERN int ZEXPORT deflatePending( z_streamp strm, unsigned* pending, int* bits );
ZEXTERN int ZEXPORT deflatePrime( z_streamp strm, int bits, int value );
ZEXTERN int ZEXPORT deflateSetHeader( z_streamp strm, gz_headerp head );

ZEXTERN int ZEXPORT inflateInit_( z_streamp strm, const char* version, int stream_size );
ZEXTERN int ZEXPORT inflateInit2_( z_streamp strm, int windowBits, const char* version,
                                   int stream_size );
ZEXTERN int ZEXPORT inflate( z_streamp strm, int flush );
ZEXTERN int ZEXPORT inflateEnd( z_streamp strm );
ZEXTERN int ZEXPORT inflateSetDictionary( z_streamp strm, const Bytef* dictionary, uInt dictLength );
ZEXTERN int ZEXPORT inflateGetDictionary( z_streamp strm, Bytef* dictionary, uInt* dictLength );
ZEXTERN int ZEXPORT inflateSync( z_streamp strm );
ZEXTERN int ZEXPORT inflateCopy( z_streamp dest, z_streamp source );
ZEXTERN int ZEXPORT inflateReset( z_streamp strm );
ZEXTERN int ZEXPORT inflateReset2( z_streamp strm, int windowBits );
ZEXTERN int ZEXPORT inflatePrime( z_streamp strm, int bits, int value );
ZEXTERN long ZEXPORT inflateMark( z_streamp strm );
ZEXTERN int ZEXPORT inflateValidate( z_streamp strm, int check );
ZEXTERN int ZEXPORT inflateGetHeader( z_streamp strm, gz_headerp head );

ZEXTERN int ZEXPORT compress( Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen );
ZEXTERN int ZEXPORT compress2( Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen,
                               int level );
ZEXTERN uLong ZEXPORT compressBound( uLong sourceLen );
ZEXTERN int ZEXPORT uncompress( Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen );
ZEXTERN int ZEXPORT uncompress2( Bytef* dest, uLongf* destLen, const Bytef* source, uLong* sourceLen );

ZEXTERN uLong ZEXPORT adler32( uLong adler, const Bytef* buf, uInt len );
ZEXTERN uLong ZEXPORT adler32_z( uLong adler, const Bytef* buf, z_size_t len );
ZEXTERN uLong ZEXPORT adler32_combine( uLong adler1, uLong adler2, z_off_t len2 );
ZEXTERN uLong ZEXPORT crc32( uLong crc, const Bytef* buf, uInt len );
ZEXTERN uLong ZEXPORT crc32_z( uLong crc, const Bytef* buf, z_size_t len );
ZEXTERN uLong ZEXPORT crc32_combine( uLong crc1, uLong crc2, z_off_t len2 );
ZEXTERN const char* ZEXPORT zError( int err );

#define deflateInit( strm, level ) deflateInit_( ( strm ), ( level ), ZLIB_VERSION, (int) sizeof( z_stream ) )
#define inflateInit( strm ) inflateInit_( ( strm ), ZLIB_VERSION, (int) sizeof( z_stream ) )
#define deflateInit2( strm, level, method, windowBits, memLevel, strategy ) \
    deflateInit2_( ( strm ), ( level ), ( method ), ( windowBits ), ( memLevel ), ( strategy ), \
                   ZLIB_VERSION, (int) sizeof( z_stream ) )
#define inflateInit2( strm, windowBits ) \
    inflateInit2_( ( strm ), ( windowBits ), ZLIB_VERSION, (int) sizeof( z_stream ) )

#ifdef __cplusplus
}
#endif

#endif
