/*
 * Internal header for libusbx
 * Copyright © 2013 Stirling Westrup <swestrup@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef ALLOCATORI_H
#define ALLOCATORI_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb.h"
#include "libusbi.h"

// This header file defines functions and macros for using the abstract
// allocation system inside libusb.

// Macro to tell GCC that a function takes printf-like arguments. A is the
// argument number (counting from 1) of the printf string, and B is the first
// argument number to compare against the string (or 0 for none).
#ifndef GCC_PRINTF
# if __GNUC__
#   define GCC_PRINTF(A,B) __attribute__ ((format (gnu_printf, A, B)))
# else
#   define GCC_PRINTF(A,B) /* nothing */
# endif
#endif

extern libusb_allocator * libusb_default_allocator;

static inline int usbi_allocator_vasprintf(
	libusb_allocator         * allocator,
	char const	         * label,
	char const	         * file,
	char const	         * func,
	long		           line,
	char			** bufp,
	const char	         * format,
	va_list			   args
) GCC_PRINTF(7,0);

static inline int usbi_allocator_asprintf(
	libusb_allocator         * allocator,
	char const	         * label,
	char const	         * file,
	char const	         * func,
	long		           line,
	char			** bufp,
	const char	         * format,
	...
) GCC_PRINTF(7,8);

// This function make calls through an Allocator to implement the current
// memory allocation policy. It is intended to be called through macros
// that generate a display label for the generated object (often the type name
// of the object being created) and capture the current source line, function,
// and filename. It adds a timestamp which is a floating point number of
// seconds since the program started.

static inline void *usbi_allocator_allocate(
	libusb_allocator * allocator,
	char const	 * label,
	char const	 * file,
	char const	 * func,
	long		   line,
	void		 * mem,
	size_t		   head,
	ulong		   count,
	size_t		   size
)
{
	return allocator->allocate(allocator->pool,label,file,func,line,
		usbi_gettimestamp(),mem,head,count,size);
}
	
static inline void *usbi_allocator_walk(
	libusb_allocator	  * allocator,
	libusb_allocator_visit_fn * visitor,
	void		          * visitorinfo
)
{
	return allocator->walk(allocator->pool,visitorinfo,visitor);
}	

// Some convenience functions, again intended to be called through macros
//
// The simpler of these are inlined, the more complex ones are defined in
// allocator.c

// Reallocate a memory area, or free it on failure.
static inline void *usbi_allocator_reallocf(
	libusb_allocator * allocator,
	char const       * label,
	char const       * file,
	char const	 * func,
	long		   line,
	void		 * mem,
	size_t		   head,
	ulong		   count,
	size_t		   size
)
{
	void * ret = usbi_allocator_allocate(allocator,label,file,func,line,
		mem, head, count, size);
	if( !ret )
		usbi_allocator_allocate(allocator,label,file,func,line,mem,0,0,0);
	return ret;
}	

static inline int usbi_allocator_vasprintf(
	libusb_allocator         * allocator,
	char const	         * label,
	char const	         * file,
	char const	         * func,
	long		           line,
	char			** bufp,
	const char	         * format,
	va_list			   args
)
{
	va_list	 		   argsc;

	va_copy(argsc, args);
	int len = vsnprintf(NULL, 0, format, argsc) + 1;
	va_end(argsc);

	char *p = usbi_allocator_allocate(allocator, label, file, func, line,
		NULL,0,len,sizeof(char));
	if (!p)
	  return -1;

	*bufp = p;
	return vsnprintf(p, len, format, args);
}


static inline int usbi_allocator_asprintf(
	libusb_allocator         * allocator,
	char const	         * label,
	char const	         * file,
	char const	         * func,
	long		           line,
	char			** bufp,
	const char	         * format,
	...
)
{
	va_list			   args;

	va_start(args, format);
	int ret = usbi_allocator_vasprintf(allocator, label, file, func, line,
		bufp, format, args);
	va_end(args);
	return ret;
}
	  

static inline char *usbi_allocator_strdup(
	libusb_allocator	* allocator,
	char const	        * label,
	char const	        * file,
	char const	        * func,
	long	       		  line,
	char const	        * str
)
{
	size_t size = strlen(str)+1;
	char * ret = usbi_allocator_allocate(allocator, label, file, func, line,
		NULL, 0, size, sizeof(char));

	if( ret )
		strcpy(ret,str);
	return ret;
}
	
static inline char *usbi_allocator_strndup(
	libusb_allocator	* allocator,
	char const	        * label,
	char const	        * file,
	char const	        * func,
	long	       		  line,
	char const	        * str,
	size_t			  len
)
{
	size_t size;
	
	for( size = 0; size < len; size++ )
		if( str[size] == '\0' )
			break;
	char * ret = usbi_allocator_allocate(allocator, label, file, func, line,
		NULL, 0, size+1, sizeof(char));

	if( ret ) {
		strncpy(ret,str,size);
		ret[size] = '\0';
	}
	return ret;
}	
	

// Label generation macros
#define _USBI_LBL1(n,t)    t "[" #n "]"
#define _USBI_LBL2(n,z,t)  t "[" #n "][" #z "]"
#define _USBI_MEM1(n)      _USBI_LBL1(n,"uint8_t")
#define _USBI_MEM2(n,z)    _USBI_LBL2(n,z,"uint8_t")
#define _USBI_HDR(h,n,t)   #h "+" #t "[" #n "]"
#define _USBI_PRV(h,z)     #h " + " #z "Bytes"


// The various usbi_raw_... allocation macros should be avoided except in
// cases where one really doesn't have a libusb_context to pass in. As these
// cases are rare, we've only created the macros we actually use.

// general allocation based on allocator.
#define usbi_raw_allocate(alc,lbl,mem,hdr,cnt,siz)	\
  usbi_allocator_allocate				\
    ( alc						\
    , lbl						\
    , __FILE__, __FUNCTION__, __LINE__			\
    , mem, hdr, cnt, siz				\
    )

// Allocate an object by size, and provide a label
#define usbi_raw_allocz(alc,lbl,siz)			\
  usbi_raw_allocate(alc,lbl,NULL,siz,0,0)

// Allocate an object by type
#define usbi_raw_alloc(alc,typ)			\
  (typ *)usbi_raw_allocate(alc,#typ,NULL,sizeof(typ),0,0)

// Free an object
#define usbi_raw_free(alc,ptr)			\
  usbi_raw_allocate(alc,NULL,ptr,0,0,0)


// general allocation based on current context.
#define usbi_allocate(ctx,lbl,mem,hdr,cnt,siz)		\
  usbi_raw_allocate(libusb_context_get_allocator(ctx),lbl,mem,hdr,cnt,siz)

// Reallocate-or-free a memory area holding an array
#define usbi_recallocf(ctx,mem,cnt,atyp)	\
  usbi_allocator_reallocf			\
    ( libusb_context_get_allocator(ctx)		\
    , _USBI_LBL1(cnt,#atyp)			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , mem, 0, cnt, sizeof(atyp)		        \
    ) 


// Reallocate-or-free a memory area with header and array
#define usbi_rehcallocf(ctx,mem,htyp,cnt,atyp)	\
  usbi_allocator_reallocf			\
    ( libusb_context_get_allocator(ctx)		\
    , _USBI_HDR(htyp,atyp,cnt)			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , mem, sizeof(htyp), cnt, sizeof(atyp)	\
    ) 


// allocate and return a formatted string.
#define usbi_asprintf(ctx,...)			\
  usbi_allocator_asprintf			\
    ( libusb_context_get_allocator(ctx)		\
    , "asprintf(" #__VA_ARGS__ ")"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , __VA_ARGS__				\
    )

// Allocate and return a vprintf'ed string
#define usbi_vasprintf(ctx,bufp,fmt,args)	\
  usbi_allocator_vasprintf			\
    ( libusb_context_get_allocator(ctx)		\
    , "vasprintf(" #fmt ", args)"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , fmt, args					\
    )

// Allocate and return the duplicate of a string.
#define usbi_strdup(ctx,str)			\
  usbi_allocator_strdup			        \
    ( libusb_context_get_allocator(ctx)		\
    , "strdup(" #str ")"			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , str					\
    )

// Allocate and return up to n characters of a string.
#define usbi_strndup(ctx,str,n)			\
  usbi_allocator_strdup			        \
    ( libusb_context_get_allocator(ctx)		\
    , "strndup(" #str "," #n ")"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , str, n					\
    )

// Allocate an object by type.
#define usbi_alloc(ctx,typ)			\
  (typ *)usbi_allocate(ctx,#typ,NULL,sizeof(typ),0,0)

// Allocate some raw memory, by size
#define usbi_allocz(ctx,siz)			\
  usbi_allocate(ctx,_USBI_MEM1(siz),NULL,siz,0,0)

// Allocate an array of objects, by type and count.
#define usbi_calloc(ctx,num,typ)		\
  (typ *)usbi_allocate(ctx,_USBI_LBL1(num,#typ),NULL,0,num,sizeof(typ))

// Allocate an array of memory chunks, by size and count
#define usbi_callocz(ctx,num,siz)		\
  usbi_allocate(ctx,_USBI_MEM2(num,siz),NULL,0,num,siz)

// Reallocate memory to be able to hold an object of given type
#define usbi_realloc(ctx,ptr,typ)		\
  usbi_allocate(ctx,#typ,ptr,sizeof(typ),0,0)

// Reallocate memory by size
#define usbi_reallocz(ctx,ptr,siz)		\
  usbi_allocate(ctx,_USBI_MEM1(siz),ptr,siz,0,0)

// Resize an array by type and number
#define usbi_recalloc(ctx,ptr,num,typ)		\
  (typ *)usbi_allocate(ctx,_USBI_LBL1(num,#typ),ptr,0,num,sizeof(typ))

// Resize an array of memory chunks by size and number
#define usbi_recallocz(ctx,ptr,num,siz)		\
  usbi_allocate(ctx,_USBI_MEM2(num,siz),ptr,0,num,siz)

// Free an allocated region of memory
#define usbi_free(ctx,ptr)			\
  usbi_allocate(ctx,NULL,ptr,0,0,0)


//
// Some even-less traditional allocation routines for convenience
//

// Allocate a memory region consisting of a header and an array.
#define usbi_hcalloc(ctx,htyp,cnt,atyp)					\
  (htyp *)usbi_allocate(ctx,_USBI_HDR(htyp,cnt,atyp),NULL,sizeof(htyp),cnt,sizeof(atyp))

// Allocate space for a header and private area of a given size.
#define usbi_hallocz(ctx,htyp,siz)					\
  (htyp *)usbi_allocate(ctx,_USBI_PRV(htyp,siz),NULL,sizeof(htyp),1,siz)


#endif
