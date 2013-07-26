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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

// Define abstract allocator, for those who want to replace libusb's
// memory allocator with their own

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

typedef void * libusb_allocator_proc_alloc
  ( void *       pool
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , size_t       size
  , ulong	 count
  );

typedef void * libusb_allocator_proc_realloc
  ( void *       pool
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , void *       mem
  , size_t       size
  , ulong	 count
  );

typedef void   libusb_allocator_proc_free
  ( void *       pool
  , char const * file
  , char const * func
  , long	 line
  , void *       mem
  );

typedef void * libusb_allocator_proc_visit
  ( void *       pool
  , void *       walkinfo
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , void *       mem
  , size_t       size
  , ulong	 count
  );

typedef void libusb_allocator_proc_walk
  ( void			* pool
  , void			* walkinfo
  , libusb_allocator_proc_visit * visit
  );

typedef struct libusb_allocator
  { void			  * pool;	    // arbitrary pool info
    libusb_allocator_proc_alloc   * alloc;
    libusb_allocator_proc_realloc * realloc;
    libusb_allocator_proc_free    * free;
    libusb_allocator_proc_walk    * walk;
  }
libusb_allocator;

// These functions make calls through an Allocator to implement the current
// memory allocation policy. They are intended to be called through macros
// that capture the current source line, function, and filename, and possibly
// the type of the object being allocated. The macros are defined at the
// bottom of this file.

void *allocator_alloc
  ( libusb_allocator * allocator
  , char const	     * name
  , char const	     * file
  , char const	     * func
  , long	       line
  , size_t	       size
  );

void *allocator_calloc
  ( libusb_allocator * allocator
  , char const       * name
  , char const       * file
  , char const       * func
  , long	       line
  , size_t	       size
  , ulong	       count
  );

void *allocator_realloc
  ( libusb_allocator * allocator
  , char const	     * name
  , char const       * file
  , char const       * func
  , long	       line
  , void	     * mem
  , size_t	       size
  );

void *allocator_recalloc
  ( libusb_allocator * allocator
  , char const	     * name
  , char const	     * file
  , char const	     * func
  , long	       line
  , void	     * mem
  , size_t	       size
  , ulong	       count
  );

void allocator_free
  ( libusb_allocator * allocator
  , char const       * file
  , char const	     * func
  , long	       line
  , void	     * mem
  );

void *allocator_walk
  ( libusb_allocator		* allocator
  , libusb_allocator_proc_visit * visitor
  , void			* visitorinfo
  );

// Some convenience functions

int allocator_vasprintf
  ( libusb_allocator * allocator
  , char const	     *  name
  , char const	     *  file
  , char const	     *  func
  , long		line
  , char	     ** bufp
  , const char       *  format
  , va_list		args
  )
GCC_PRINTF(7,0);

int allocator_asprintf
  ( libusb_allocator * allocator
  , char const	     *  name
  , char const	     *  file
  , char const	     *  func
  , long		line
  , char	     ** bufp
  , const char	     *  format
  , ...
  )
GCC_PRINTF(7,8);

char *allocator_strdup
  ( libusb_allocator * allocator
  , char const	     * name
  , char const	     * file
  , char const	     * func
  , long	       line
  , char const	     * str
  );


extern libusb_allocator const libusb_default_allocator;

// Allocate an object by type.
#define usbi_alloc(ctx,typ)			\
  (typ *) allocator_alloc			\
    ( libusb_context_get_allocator(ctx)		\
    , #typ					\
    , __FILE__, __FUNCTION__, __LINE__		\
    , sizeof(typ)				\
    )

// Allocate some raw memory, by size		\
#define usbi_allocz(ctx,siz)			\
  (void *) allocpolicy_alloc			\
    ( libusb_context_get_allocator(ctx)		\
    , "uint_8[" #siz "]"			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , siz					\
    )

// Allocate an array of objects, by type and count.
#define usbi_calloc(ctx,num,typ)		\
  (typ *) allocator_calloc			\
    ( libusb_context_get_allocator(ctx)		\
    , #typ "[" #num "]"				\
    , __FILE__, __FUNCTION__, __LINE__		\
    , (ulong)(num)				\
    , sizeof(typ)				\
    )

// Allocate an array of memory chunks, by size and count
#define usbi_callocz(ctx,num,siz)		\
  (void *) allocpolicy_calloc			\
    ( libusb_context_get_allocator(ctx)		\
    , "Byte[" #num "][" #siz "]"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , (ulong)(num)				\
    , siz					\
    )

// Reallocate memory to be able to hold an object of given type
#define usbi_realloc(ctx,ptr,typ)		\
  (typ *) allocpolicy_realloc			\
    ( libusb_context_get_allocator(ctx)		\
    , #typ					\
    , __FILE__, __FUNCTION__, __LINE__		\
    , ptr					\
    , sizeof(typ)				\
    )

// Reallocate memory by size
#define usbi_reallocz(ctx,ptr,siz)		\
  (Byte *) allocpolicy_realloc			\
    ( libusb_context_get_allocator(ctx)		\
    , "Byte[" #siz "]"				\
    , __FILE__, __FUNCTION__, __LINE__		\
    , ptr					\
    , siz					\
    )

// Resize an array by type and number
#define usbi_recalloc(ctx,ptr,num,typ)		\
  (typ *) allocpolicy_recalloc			\
    ( libusb_context_get_allocator(ctx)		\
    , #typ "[" #num "]"				\
    , __FILE__, __FUNCTION__, __LINE__		\
    , ptr					\
    , (ulong)(num)				\
    , sizeof(typ)				\
    )

// Resize an array of memory chunks by size and type
#define usbi_recallocz(ctx,ptr,num,siz)		\
  (Byte *)allocpolicy_recalloc			\
    ( libusb_context_get_allocator(ctx)		\
    , "Byte[" #num "][" #siz "]"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , ptr					\
    , (ulong)(num)				\
    , siz					\
    )

// Free an allocated region of memory
#define usbi_free(ctx,ptr)			\
  allocpolicy_free				\
    ( libusb_context_get_allocator(ctx)		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , ptr					\
    )

// allocate and return a formatted string.
#define usbi_asprintf(ctx,bufp,...)		\
  allocpolicy_asprintf				\
    ( libusb_context_get_allocator(ctx)		\
    , "asprintf(" #__VA_ARGS__ ")"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , __VA_ARGS__
    )

// Allocate and return a vprintf'ed string
#define usbi_vasprintf(ctx,bufp,fmt,args)	\
  allocpolicy_vasprintf				\
    ( libusb_context_get_allocator(ctx)		\
    , "vasprintf(" #fmt ", args)"		\
    , __FILE__, __FUNCTION__, __LINE__		\
    , fmt, args					\
    )

// Allocate and return the duplicate of a string.
#define usbi_strdup(ctx,str)			\
  allocpolicy_strdup				\
    ( libusb_context_get_allocator(ctx)		\
    , "strdup(" #str ")"			\
    , __FILE__, __FUNCTION__, __LINE__		\
    , str					\
    )

#endif
