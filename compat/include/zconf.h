/* zlib's basic types, for zlib.h over z.library (gfamiga_zlib.c).
 * Copyright (c) 2026 AmiCradle Team. MIT. */
#ifndef GFAMIGA_ZCONF_H
#define GFAMIGA_ZCONF_H

#include <stddef.h>

#if defined( ZLIB_CONST ) && !defined( z_const )
#define z_const const
#elif !defined( z_const )
#define z_const
#endif

#define ZEXTERN extern
#define ZEXPORT
#define ZEXPORTVA
#define OF(args) args
#define z_off_t long
#define z_off64_t long long
#define MAX_MEM_LEVEL 9
#define MAX_WBITS 15
#ifndef FAR
#define FAR
#endif

typedef unsigned char  Byte;
typedef unsigned int   uInt;
typedef unsigned long  uLong;
typedef Byte  FAR      Bytef;
typedef char  FAR      charf;
typedef int   FAR      intf;
typedef uInt  FAR      uIntf;
typedef uLong FAR      uLongf;
typedef void const*    voidpc;
typedef void FAR*      voidpf;
typedef void*          voidp;
typedef unsigned long  z_crc_t;
typedef size_t         z_size_t;

#endif
