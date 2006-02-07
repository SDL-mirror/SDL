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

#ifdef HAVE_MEMSET
#define SDL_memset      memset
#else
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
#endif
#ifndef SDL_memcpy
#ifdef HAVE_MEMCPY
#define SDL_memcpy      memcpy
#else
extern DECLSPEC void * SDLCALL SDL_memcpy(void *dst, const void *src, size_t len);
#endif
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

#ifdef HAVE_MEMMOVE
#define SDL_memmove     memmove
#else
#define SDL_memmove(dst, src, len)			\
do {							\
	if ( dst < src ) {				\
		SDL_memcpy(dst, src, len);		\
	} else {					\
		SDL_revcpy(dst, src, len);		\
	}						\
} while(0)
#endif

#ifdef HAVE_MEMCMP
#define SDL_memcmp      memcmp
#else
extern DECLSPEC int SDLCALL SDL_memcmp(const void *s1, const void *s2, size_t len);
#endif

#ifdef HAVE_STRLEN
#define SDL_strlen      strlen
#else
extern DECLSPEC size_t SDLCALL SDL_strlen(const char *string);
#endif

#ifdef HAVE_STRCPY
#define SDL_strcpy     strcpy
#else
extern DECLSPEC char * SDLCALL SDL_strcpy(char *dst, const char *src);
#endif

#ifdef HAVE_STRNCPY
#define SDL_strncpy     strncpy
#else
extern DECLSPEC char * SDLCALL SDL_strncpy(char *dst, const char *src, size_t maxlen);
#endif

#ifdef HAVE_STRCAT
#define SDL_strcat     strcat
#else
#define SDL_strcat(dst, src)    (SDL_strcpy(dst+SDL_strlen(dst), src), dst)
#endif

#ifdef HAVE_STRNCAT
#define SDL_strncat    strncat
#else
#define SDL_strncat(dst, src, n) (SDL_strncpy(dst+SDL_strlen(dst), src, n), dst)
#endif

#ifdef HAVE_STRDUP
#define SDL_strdup     strdup
#else
extern DECLSPEC char * SDLCALL SDL_strdup(const char *string);
#endif

#ifdef HAVE__STRREV
#define SDL_strrev      _strrev
#else
extern DECLSPEC char * SDLCALL SDL_strrev(char *string);
#endif

#ifdef HAVE__STRUPR
#define SDL_strupr      _strupr
#else
extern DECLSPEC char * SDLCALL SDL_strupr(char *string);
#endif

#ifdef HAVE__STRLWR
#define SDL_strlwr      _strlwr
#else
extern DECLSPEC char * SDLCALL SDL_strlwr(char *string);
#endif

#ifdef HAVE_STRCHR
#define SDL_strchr      strchr
#else
extern DECLSPEC char * SDLCALL SDL_strchr(const char *string, int c);
#endif

#ifdef HAVE_STRRCHR
#define SDL_strrchr     strrchr
#else
extern DECLSPEC char * SDLCALL SDL_strrchr(const char *string, int c);
#endif

#ifdef HAVE_STRSTR
#define SDL_strstr      strstr
#else
extern DECLSPEC char * SDLCALL SDL_strstr(const char *haystack, const char *needle);
#endif

#ifdef HAVE_ITOA
#define SDL_itoa        itoa
#else
#define SDL_itoa(value, string, radix)	SDL_ltoa((long)value, string, radix)
#endif

#ifdef HAVE__LTOA
#define SDL_ltoa        _ltoa
#else
extern DECLSPEC char * SDLCALL SDL_ltoa(long value, char *string, int radix);
#endif

#ifdef HAVE__UITOA
#define SDL_uitoa       _uitoa
#else
#define SDL_uitoa(value, string, radix)	SDL_ultoa((long)value, string, radix)
#endif

#ifdef HAVE__ULTOA
#define SDL_ultoa       _ultoa
#else
extern DECLSPEC char * SDLCALL SDL_ultoa(unsigned long value, char *string, int radix);
#endif

#ifdef HAVE_STRTOL
#define SDL_strtol      strtol
#else
extern DECLSPEC long SDLCALL SDL_strtol(const char *string, char **endp, int base);
#endif

#ifdef SDL_HAS_64BIT_TYPE

#ifdef HAVE__I64TOA
#define SDL_lltoa       _i64toa
#else
extern DECLSPEC char* SDLCALL SDL_lltoa(Sint64 value, char *string, int radix);
#endif

#ifdef HAVE__UI64TOA
#define SDL_ulltoa      _ui64toa
#else
extern DECLSPEC char* SDLCALL SDL_ulltoa(Uint64 value, char *string, int radix);
#endif

#ifdef HAVE_STRTOLL
#define SDL_strtoll     strtoll
#else
extern DECLSPEC Sint64 SDLCALL SDL_strtoll(const char *string, char **endp, int base);
#endif

#endif /* SDL_HAS_64BIT_TYPE */

#ifdef HAVE_STRTOD
#define SDL_strtod      strtod
#else
extern DECLSPEC double SDLCALL SDL_strtod(const char *string, char **endp);
#endif

#ifdef HAVE_ATOI
#define SDL_atoi        atoi
#else
#define SDL_atoi(X)     SDL_strtol(X, NULL, 0)
#endif

#ifdef HAVE_ATOF
#define SDL_atof        atof
#else
#define SDL_atof(X)     SDL_strtod(X, NULL)
#endif

#ifdef HAVE_STRCMP
#define SDL_strcmp      strcmp
#else
extern DECLSPEC int SDLCALL SDL_strcmp(const char *str1, const char *str2);
#endif

#ifdef HAVE_STRNCMP
#define SDL_strncmp     strncmp
#else
extern DECLSPEC int SDLCALL SDL_strncmp(const char *str1, const char *str2, size_t maxlen);
#endif

#if defined(HAVE_STRICMP) && !defined(HAVE_STRCASECMP)
#define strcasecmp      stricmp
#define HAVE_STRCASECMP
#endif
#ifdef HAVE_STRCASECMP
#define SDL_strcasecmp  strcasecmp
#else
extern DECLSPEC int SDLCALL SDL_strcasecmp(const char *str1, const char *str2);
#endif

#ifdef HAVE_SSCANF
#define SDL_sscanf      sscanf
#else
extern DECLSPEC int SDLCALL SDL_sscanf(const char *text, const char *fmt, ...);
#endif

#ifdef HAVE_SNPRINTF
#define SDL_snprintf    snprintf
#else
extern DECLSPEC int SDLCALL SDL_snprintf(char *text, size_t maxlen, const char *fmt, ...);
#endif

#ifdef HAVE_VSNPRINTF
#define SDL_vsnprintf   vsnprintf
#else
extern DECLSPEC int SDLCALL SDL_vsnprintf(char *text, size_t maxlen, const char *fmt, va_list ap);
#endif

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif
#include "close_code.h"

#endif /* _SDL_string_h */
