/* Included first in every file of the KiCad build (-include).
   libpthread's <pthread.h> wraps read(), close(), open() and other
   cancellation points in function-like macros when <unistd.h> or <fcntl.h>
   came first; in C++ they break std::istream::read and every member named
   like them. Taking <pthread.h> first means the macros are never made.
   Copyright (c) 2026 AmiCradle Team */
#ifndef GFAMIGA_PREFIX_H
#define GFAMIGA_PREFIX_H
/* -std=c++20/c99 without GNU extensions hides the BSD and POSIX calls
   (readlink, strdup ...) wx and KiCad use; g++ on Linux gets them through
   its implied _GNU_SOURCE, so ask for the same here. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#include <pthread.h>
/* <sys/types.h> makes major() and minor() macros, which break members named
   major and minor (BS::thread_pool's version, wx's version info). */
#include <sys/types.h>
#undef major
#undef minor
/* libnix has no O_CLOEXEC; AmigaOS never execs, so it means nothing. */
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#undef close
#undef fsync
#undef read
#undef sleep
#undef usleep
#undef creat
#undef fcntl
#undef open
#undef nanosleep
#undef readv
#undef writev
#undef write
#endif
