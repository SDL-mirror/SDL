/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/* This file contains portable string manipulation functions for SDL */

#ifndef _SDL_string_h
#define _SDL_string_h

#include "SDL_config.h"

#ifdef HAVE_STDIO_H
#include <stdio.h>	/* For snprintf() and friends */
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "SDL_types.h"
#include "SDL_stdarg.h"

#include "begin_code.h"
/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_MEMSET
#define memset          SDL_memset
#endif
#ifndef SDL_memset
extern DECLSPEC void * SDLCALL SDL_memset(void *dst, int c, size_t len);
#endif

#if defined(__GNUC__) && defined(i386)
#define SDL_memset4(dst, val, len)				\
do {								\
	int u0, u1, u2;						\
	__asm__ __volatile__ (					\
		"cld\n\t"					\
		"rep ; stosl\n\t"				\
		: "=&D" (u0), "=&a" (u1), "=&c" (u2)		\
		: "0" (dst), "1" (val), "2" ((Uint32)(len))	\
		: "memory" );					\
} while(0)
#endif
#ifndef SDL_memset4
#define SDL_memset4(dst, val, len)		\
do {						\
	unsigned _count = (len);		\
	unsigned _n = (_count + 3) / 4;		\
	Uint32 *_p = (Uint32 *)(dst);		\
	Uint32 _val = (val);			\
        switch (_count % 4) {			\
        case 0: do {    *_p++ = _val;		\
        case 3:         *_p++ = _val;		\
        case 2:         *_p++ = _val;		\
        case 1:         *_p++ = _val;		\
		} while ( --_n );		\
	}					\
} while(0)
#endif

#if defined(__GNUC__) && defined(i386)
#define SDL_memcpy(dst, src, len)					  \
do {									  \
	int u0, u1, u2;						  	  \
	__asm__ __volatile__ (						  \
		"cld\n\t"						  \
		"rep ; movsl\n\t"					  \
		"testb $2,%b4\n\t"					  \
		"je 1f\n\t"						  \
		"movsw\n"						  \
		"1:\ttestb $1,%b4\n\t"					  \
		"je 2f\n\t"						  \
		"movsb\n"						  \
		"2:"							  \
		: "=&c" (u0), "=&D" (u1), "=&S" (u2)			  \
		: "0" ((unsigned)(len)/4), "q" (len), "1" (dst),"2" (src) \
		: "memory" );						  \
} while(0)
#define SDL_memcpy4(dst, src, len)				\
do {								\
	int ecx, edi, esi;					\
	__asm__ __volatile__ (					\
		"cld\n\t"					\
		"rep ; movsl"					\
		: "=&c" (ecx), "=&D" (edi), "=&S" (esi)		\
		: "0" ((unsigned)(len)), "1" (dst), "2" (src)	\
		: "memory" );					\
} while(0)
#endif
#ifndef HAVE_MEMCPY
#define memcpy          SDL_memcpy
#endif
#ifndef SDL_memcpy
extern DECLSPEC void * SDLCALL SDL_memcpy(void *dst, const void *src, size_t len);
#endif

#if defined(__GNUC__) && defined(i386)
#define SDL_memcpy4(dst, src, len)				\
do {								\
	int ecx, edi, esi;					\
	__asm__ __volatile__ (					\
		"cld\n\t"					\
		"rep ; movsl"					\
		: "=&c" (ecx), "=&D" (edi), "=&S" (esi)		\
		: "0" ((unsigned)(len)), "1" (dst), "2" (src)	\
		: "memory" );					\
} while(0)
#endif
#ifndef SDL_memcpy4
#define SDL_memcpy4(dst, src, len)	SDL_memcpy(dst, src, (len) << 2)
#endif

#if defined(__GNUC__) && defined(i386)
#define SDL_revcpy(dst, src, len)			\
do {							\
	int u0, u1, u2;					\
	char *dstp = (char *)(dst);			\
	char *srcp = (char *)(src);			\
	int n = (len);					\
	if ( n >= 4 ) {					\
	__asm__ __volatile__ (				\
		"std\n\t"				\
		"rep ; movsl\n\t"			\
		: "=&c" (u0), "=&D" (u1), "=&S" (u2)	\
		: "0" (n >> 2),				\
		  "1" (dstp+(n-4)), "2" (srcp+(n-4))	\
		: "memory" );				\
	}						\
	switch (n & 3) {				\
		case 3: dstp[2] = srcp[2];		\
		case 2: dstp[1] = srcp[1];		\
		case 1: dstp[0] = srcp[0];		\
			break;				\
		default:				\
			break;				\
	}						\
} while(0)
#endif
#ifndef SDL_revcpy
extern DECLSPEC void * SDLCALL SDL_revcpy(void *dst, const void *src, size_t len);
#endif

#ifndef HAVE_MEMMOVE
#define memmove         SDL_memmove
#endif
#define SDL_memmove(dst, src, len)			\
do {							\
	if ( dst < src ) {				\
		SDL_memcpy(dst, src, len);		\
	} else {					\
		SDL_revcpy(dst, src, len);		\
	}						\
} while(0)

#ifndef HAVE_MEMCMP
#define memcmp          SDL_memcmp
#endif
#ifndef SDL_memcmp
extern DECLSPEC int SDLCALL SDL_memcmp(const void *s1, const void *s2, size_t len);
#endif

#ifdef HAVE_STRLEN
#define SDL_strlen      strlen
#else
#define strlen          SDL_strlen
extern DECLSPEC size_t SDLCALL SDL_strlen(const char *string);
#endif

#ifdef HAVE_STRCPY
#define SDL_strcpy     strcpy
#else
#define strcpy         SDL_strcpy
extern DECLSPEC char * SDLCALL SDL_strcpy(char *dst, const char *src);
#endif

#ifdef HAVE_STRNCPY
#define SDL_strncpy     strncpy
#else
#define strncpy         SDL_strncpy
extern DECLSPEC char * SDLCALL SDL_strncpy(char *dst, const char *src, size_t maxlen);
#endif

#ifdef HAVE__STRREV
#define SDL_strrev      _strrev
#else
#define _strrev	        SDL_strrev
extern DECLSPEC char * SDLCALL SDL_strrev(char *string);
#endif

#ifdef HAVE__STRUPR
#define SDL_strupr      _strupr
#else
#define _strupr         SDL_strupr
extern DECLSPEC char * SDLCALL SDL_strupr(char *string);
#endif

#ifdef HAVE__STRLWR
#define SDL_strlwr      _strlwr
#else
#define _strlwr         SDL_strlwr
extern DECLSPEC char * SDLCALL SDL_strlwr(char *string);
#endif

#ifdef HAVE_STRCHR
#define SDL_strchr      strchr
#else
#define strchr          SDL_strchr
extern DECLSPEC char * SDLCALL SDL_strchr(const char *string, int c);
#endif

#ifdef HAVE_STRRCHR
#define SDL_strrchr     strrchr
#else
#define strrchr         SDL_strrchr
extern DECLSPEC char * SDLCALL SDL_strrchr(const char *string, int c);
#endif

#ifdef HAVE_STRSTR
#define SDL_strstr      strstr
#else
#define strstr          SDL_strstr
extern DECLSPEC char * SDLCALL SDL_strstr(const char *haystack, const char *needle);
#endif

#ifdef HAVE_ITOA
#define SDL_itoa        itoa
#else
#define itoa            SDL_itoa
#define SDL_itoa(value, string, radix)	SDL_ltoa((long)value, string, radix)
#endif

#ifdef HAVE__LTOA
#define SDL_ltoa        _ltoa
#else
#define _ltoa        SDL_ltoa
extern DECLSPEC char * SDLCALL SDL_ltoa(long value, char *string, int radix);
#endif

#ifdef HAVE__UITOA
#define SDL_uitoa       _uitoa
#else
#define _uitoa          SDL_uitoa
#define SDL_uitoa(value, string, radix)	SDL_ultoa((long)value, string, radix)
#endif

#ifdef HAVE__ULTOA
#define SDL_ultoa       _ultoa
#else
#define _ultoa          SDL_ultoa
extern DECLSPEC char * SDLCALL SDL_ultoa(unsigned long value, char *string, int radix);
#endif

#ifdef HAVE_STRTOL
#define SDL_strtol      strtol
#else
#define strtol          SDL_strtol
extern DECLSPEC long SDLCALL SDL_strtol(const char *string, char **endp, int base);
#endif

#ifdef SDL_HAS_64BIT_TYPE

#ifdef HAVE__I64TOA
#define SDL_lltoa       _i64toa
#else
#define _i64toa         SDL_lltoa
extern DECLSPEC char* SDLCALL SDL_lltoa(Sint64 value, char *string, int radix);
#endif

#ifdef HAVE__UI64TOA
#define SDL_ulltoa      _ui64toa
#else
#define _ui64toa        SDL_ulltoa
extern DECLSPEC char* SDLCALL SDL_ulltoa(Uint64 value, char *string, int radix);
#endif

#ifdef HAVE_STRTOLL
#define SDL_strtoll     strtoll
#else
#define strtoll         SDL_strtoll
extern DECLSPEC Sint64 SDLCALL SDL_strtoll(const char *string, char **endp, int base);
#endif

#endif /* SDL_HAS_64BIT_TYPE */

#ifdef HAVE_STRCMP
#define SDL_strcmp      strcmp
#else
#define strcmp          SDL_strcmp
extern DECLSPEC int SDLCALL SDL_strcmp(const char *str1, const char *str2);
#endif

#ifdef HAVE_STRNCMP
#define SDL_strncmp     strncmp
#else
#define strncmp         SDL_strncmp
extern DECLSPEC int SDLCALL SDL_strncmp(const char *str1, const char *str2, size_t maxlen);
#endif

#if defined(HAVE_STRICMP) && !defined(HAVE_STRCASECMP)
#define strcasecmp      stricmp
#define HAVE_STRCASECMP
#endif
#ifdef HAVE_STRCASECMP
#define SDL_strcasecmp  strcasecmp
#else
#define strcasecmp      SDL_strcasecmp
extern DECLSPEC int SDLCALL SDL_strcasecmp(const char *str1, const char *str2);
#endif

#ifdef HAVE_SSCANF
#define SDL_sscanf      sscanf
#else
#define sscanf          SDL_sscanf
extern DECLSPEC int SDLCALL SDL_sscanf(const char *text, const char *fmt, ...);
#endif

#ifdef HAVE_SNPRINTF
#define SDL_snprintf    snprintf
#else
#define snprintf        SDL_snprintf
extern DECLSPEC int SDLCALL SDL_snprintf(char *text, size_t maxlen, const char *fmt, ...);
#endif

#ifdef HAVE_VSNPRINTF
#define SDL_vsnprintf   vsnprintf
#else
#define vsnprintf       SDL_vsnprintf
extern DECLSPEC int SDLCALL SDL_vsnprintf(char *text, size_t maxlen, const char *fmt, va_list ap);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_string_h */
