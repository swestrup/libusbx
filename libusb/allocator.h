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

// #include "macros.h"

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
// that capture the current source line, function, and filename

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
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , char **       bufp
  , const char *  format
  , ...
  )
GCC_PRINTF(7,8);

char *allocator_strdup
  ( libusb_allocator * allocator
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , char const *  str
  );


// Note: This logging policy should actually have an embedded concept of
// output logging streams, and ways to attach/detach and enable/disable
// streams based on predicate callbacks so that, for instance, the stdout
// stream responds to minor messages only, the stderr stream responds to major
// messages, and the file stream takes everything. Thus, it seems we need an
// output logging stream abstraction consisting of: an output stream, a local
// logging level, local logging flags, local output data, and a predicate
// callback that takes the local level and flags and the message'sss level and
// flags, and returns a boolean indicating if that stream should output the
// message or not. However this is way overkill for what we are doing right
// now.

typedef enum
  { LOGLEVEL_ERROR
  , LOGLEVEL_WARNING
  , LOGLEVEL_INFO
  , LOGLEVEL_NOTICE
  , LOGLEVEL_DEBUG
  , LOGLEVEL_TRACE
  , LOGLEVEL_
  }
LogLevel;

typedef uint32_t LogFlags;
#define LOGFLAGS_ALL UINT32_MAX

typedef void LogBegProc
  ( void *       data
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  );

typedef void LogLogProc
  ( void *       data
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  , char const * format
  , va_list      args
  )
GCC_PRINTF(7,0);

typedef void LogEndProc
  ( void *       data
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  );

typedef LogLevel LogGetLevelProc(void *data);
typedef void     LogSetLevelProc(void *data, LogLevel level);
typedef LogFlags LogGetFlagsProc(void *data);
typedef void     LogSetFlagsProc(void *data, LogFlags flags);

typedef struct LogPolicy
  { void *	      data;	    // arbitrary log control cata
    LogBegProc	    * beg;	    // Begin log entry with header
    LogLogProc	    * log;	    // Output some log entry content
    LogEndProc	    * end;	    // Terminate this log entry.
    LogGetLevelProc * get_level;    // Get the current logging level
    LogSetLevelProc * set_level;    // Set the current logging level
    LogGetFlagsProc * get_flags;    // Get the currnet logging flags
    LogSetFlagsProc * set_flags;    // Set the current logging flags
  }
LogPolicy;

// These functions make calls through a logpolicy to implement the current
// logging policy.

// Acquire any needed locks on and/or open output streams, and output any
// initial header info for a log entry.
void logpolicy_beg
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  );

// Output some or all of a log header and/or entry.
void logpolicy_vmid
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  , char const * format
  , va_list      args
  )
GCC_PRINTF(7,0);

// Output some or all of a log header and/or entry.
void logpolicy_mid
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  , char const * format
  , ...
  )
GCC_PRINTF(7,8);

// Output any trailing log entry info, and optionally flush and close streams
// and release any output locks.
void logpolicy_end
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  );

// Convenience function that combines the beg, mid, and end functions above.
void logpolicy_log
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  , char const * format
  , ...
  )
GCC_PRINTF(7,8);

// Convenience function that combines the beg, vmid, and end functions above.
void logpolicy_vlog
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  , char const * format
  , va_list      args
  )
GCC_PRINTF(7,0);

// Convenience function that combines bed and end, but no message, for tracing.
void logpolicy_trace
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  );

extern LogPolicy        const logPolicy_default;
extern libusb_allocator const allocator_default;

#endif
