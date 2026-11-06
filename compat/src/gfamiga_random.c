/*
 * /dev/urandom and /dev/random for programs linked with
 * -Wl,--wrap=open -Wl,--wrap=read -Wl,--wrap=close.
 *
 * GCC's std::random_device (and so Boost's uuid generator, which KiCad
 * makes every item's id with) opens /dev/urandom. AmigaOS has no such
 * file: libnix turns the path into a volume name and the open waits for
 * ever on a "please insert volume" request. These wrappers answer those
 * two paths themselves, with bytes from a generator seeded from the clock,
 * the task, the stack and free memory; every other path goes to libnix.
 * Good enough for ids and seeds, not for keys.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

int     __real_open( const char* path, int flags, ... );
ssize_t __real_read( int fd, void* buf, size_t count );
int     __real_close( int fd );

/* far above any descriptor libnix hands out */
#define RANDOM_FD 0x7f00

static unsigned long long state;
static int                seeded;

static unsigned long long mix( unsigned long long z )
{
    z = ( z ^ ( z >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
    z = ( z ^ ( z >> 27 ) ) * 0x94d049bb133111ebULL;
    return z ^ ( z >> 31 );
}

static void seed( void )
{
    struct DateStamp ds;
    int              here;

    DateStamp( &ds );
    state = mix( (unsigned long long) ds.ds_Days << 40 ^ (unsigned long long) ds.ds_Minute << 20
                 ^ (unsigned long long) ds.ds_Tick );
    state = mix( state ^ (unsigned long long) (unsigned long) time( NULL ) );
    state = mix( state ^ (unsigned long long) (unsigned long) clock() << 16 );
    state = mix( state ^ (unsigned long long) (unsigned long) FindTask( NULL ) << 8 );
    state = mix( state ^ (unsigned long long) (unsigned long) &here );
    state = mix( state ^ (unsigned long long) AvailMem( MEMF_ANY ) << 24 );
    seeded = 1;
}

static int is_random_path( const char* path )
{
    return path && ( strcmp( path, "/dev/urandom" ) == 0 || strcmp( path, "/dev/random" ) == 0 );
}

int __wrap_open( const char* path, int flags, ... )
{
    va_list ap;
    int     mode;

    if( is_random_path( path ) )
    {
        if( !seeded )
            seed();

        return RANDOM_FD;
    }

    va_start( ap, flags );
    mode = va_arg( ap, int );
    va_end( ap );
    return __real_open( path, flags, mode );
}

ssize_t __wrap_read( int fd, void* buf, size_t count )
{
    if( fd == RANDOM_FD )
    {
        unsigned char* out = (unsigned char*) buf;
        size_t         i;

        for( i = 0; i < count; i++ )
        {
            if( ( i & 7 ) == 0 )
                state = mix( state + 0x9e3779b97f4a7c15ULL );

            out[i] = (unsigned char) ( state >> ( ( i & 7 ) * 8 ) );
        }

        return (ssize_t) count;
    }

    return __real_read( fd, buf, count );
}

int __wrap_close( int fd )
{
    if( fd == RANDOM_FD )
        return 0;

    return __real_close( fd );
}
