#ifndef ROMCNV_H
#define ROMCNV_H

//#undef WIN32

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#ifdef WIN32
#define COBJMACROS
#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <conio.h>
#else
#include <unistd.h>
#endif
#include <zlib.h>

#ifndef MAX_PATH
#define MAX_PATH	256
#endif

#ifdef WIN32
#define chdir _chdir
#define mkdir _mkdir
#define getcwd _getcwd
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long  u64;
typedef char				s8;
typedef short				s16;
typedef int					s32;
typedef signed long long    s64;

#include "zfile.h"

#endif /* ROMCNV_H */
