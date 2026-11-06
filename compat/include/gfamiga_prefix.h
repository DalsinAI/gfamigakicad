/* Included first in every file of the KiCad build (-include).
   libpthread's <pthread.h> wraps read(), close(), open() and other
   cancellation points in function-like macros when <unistd.h> or <fcntl.h>
   came first; in C++ they break std::istream::read and every member named
   like them. Taking <pthread.h> first means the macros are never made.
   Copyright (c) 2026 AmiCradle Team */
#ifndef GFAMIGA_PREFIX_H
#define GFAMIGA_PREFIX_H
#include <pthread.h>
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
