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
#include <stdlib.h>

#include "libusb.h"

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

extern libusb_allocator const libusb_default_allocator;

// This function make calls through an Allocator to implement the current
// memory allocation policy. It is intended to be called through macros
// that generate a display label for the generated object (often the type name
// of the object being created) and capture the current source line, function,
// and filename. It adds a timestamp which is a floating point number of
// seconds since the program started.

void *libusb_allocator_allocate
  ( libusb_allocator * allocator
  , char const	     * label
  , char const	     * file
  , char const	     * func
  , long	       line
  , void	     * mem
  , ulong	       count
  , size_t	       size
  );

void *libusb_allocator_walk
  ( libusb_allocator	      * allocator
  , libusb_allocator_visit_fn * visitor
  , void		      * visitorinfo
  );

// Some convenience functions, again intended to be called through macros

int libusb_allocator_vasprintf
  ( libusb_allocator *  allocator
  , char const	     *  label
  , char const	     *  file
  , char const	     *  func
  , long		line
  , char	     ** bufp
  , const char       *  format
  , va_list		args
  )
GCC_PRINTF(7,0);

int libusb_allocator_asprintf
  ( libusb_allocator *  allocator
  , char const	     *  label
  , char const	     *  file
  , char const	     *  func
  , long		line
  , char	     ** bufp
  , const char	     *  format
  , ...
  )
GCC_PRINTF(7,8);

char *libusb_allocator_strdup
  ( libusb_allocator * allocator
  , char const	     * label
  , char const	     * file
  , char const	     * func
  , long	       line
  , char const	     * str
  );

extern libusb_allocator const libusb_default_allocator;

// Allocation macro using specified allocator. This is only needed in cases
// where we don't yet have a context.
#define usbi_raw_allocate(alc,lbl,mem,cnt,siz)	\
  libusb_allocator_allocate			\
    ( alc					\
    , lbl					\
    , __FILE__, __FUNCTION__, __LINE__		\
    , mem, cnt, siz				\
    )

// general allocation based on current context.
#define usbi_allocate(ctx,lbl,mem,cnt,siz)	\
  usbi_raw_allocate(libusb_context_get_allocator(ctx),lbl,mem,cnt,siz)

// allocate and return a formatted string.
#define usbi_asprintf(ctx,...)			\
  libusb_allocator_asprintf			\
    ( libusb_context_get_allocator(ctx)		\
    , "asprintf(" #__VA_ARGS__ ")"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , __VA_ARGS__				\
    )

// Allocate and return a vprintf'ed string
#define usbi_vasprintf(ctx,bufp,fmt,args)	\
  libusb_allocator_vasprintf			\
    ( libusb_context_get_allocator(ctx)		\
    , "vasprintf(" #fmt ", args)"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , fmt, args					\
    )

// Allocate and return the duplicate of a string.
#define usbi_strdup(ctx,str)			\
  libusb_allocator_strdup			\
    ( libusb_context_get_allocator(ctx)		\
    , "strdup(" #str ")"			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , str					\
    )


// Label generation macros
#define _LBL1(n,t)    t "[" #n "]"
#define _LBL2(n,z,t)  t "[" #n "][" #z "]"
#define _MEM1(n)      _LBL1(n,"uint_8")
#define _MEM2(n,z)    _LBL2(n,z,"uint_8")

// Allocate an object by type.
#define usbi_alloc(ctx,typ)			\
  (typ *)usbi_allocate(ctx,#typ,NULL,1,sizeof(typ))

// Allocate some raw memory, by size
#define usbi_allocz(ctx,siz)			\
  usbi_allocate(ctx,_MEM1(siz),NULL,1,siz)

// Allocate an array of objects, by type and count.
#define usbi_calloc(ctx,num,typ)		\
  (typ *)usbi_allocate(ctx,_LBL1(num,#typ),NULL,num,sizeof(typ))

// Allocate an array of memory chunks, by size and count
#define usbi_callocz(ctx,num,siz)		\
  usbi_allocate(ctx,_MEM2(num,siz),NULL,num,sizeof(typ))

// Reallocate memory to be able to hold an object of given type
#define usbi_realloc(ctx,ptr,typ)		\
  usbi_allocate(ctx,#typ,ptr,1,sizeof(typ))

// Reallocate memory by size
#define usbi_reallocz(ctx,ptr,siz)		\
  usbi_allocate(ctx,_MEM1(siz),ptr,1,siz)

// Resize an array by type and number
#define usbi_recalloc(ctx,ptr,num,typ)		\
  (typ *)usbi_allocate(ctx,_LBL1(num,#typ),ptr,num,sizeof(typ))

// Resize an array of memory chunks by size and type
#define usbi_recallocz(ctx,ptr,num,siz)		\
  usbi_allocate(ctx,_MEM2(num,siz),ptr,num,sizeof(typ))

// Free an allocated region of memory
#define usbi_free(ctx,ptr)			\
  usbi_allocate(ctx,NULL,ptr,0,0)

#endif
