/*
 * Copyright 2012 Userful Corporation.
 */

#ifndef _DL_POLICY_H
#define _DL_POLICY_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "macros.h"

// Define abstract allocation and logging policies, for concrete
// instances to conform to.

typedef unsigned long unsigned_long;

#if !defined(ulong)
  #define ulong unsigned_long
#endif

typedef void * PoolAllocProc
  ( void *       pool
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , size_t       size
  , ulong	 count
  );

typedef void * PoolReallocProc
  ( void *       pool
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , void *       mem
  , size_t       size
  , ulong	 count
  );

typedef void   PoolFreeProc
  ( void *       pool
  , char const * file
  , char const * func
  , long	 line
  , void *       mem
  );

typedef void * PoolVisitProc
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

typedef void   PoolWalkProc
  ( void *	    pool
  , void *	    walkinfo
  , PoolVisitProc * entry
  );

typedef struct AllocPolicy
  { void	    * pool;	    // arbitrary pool info
    PoolAllocProc   * alloc;
    PoolReallocProc * realloc;
    PoolFreeProc    * free;
    PoolWalkProc    * walk;
  }
AllocPolicy;

// These functions make calls through an AllocPolicy to implement the current
// memory allocation policy.

void *allocpolicy_alloc
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , size_t	  size
  );

void *allocpolicy_calloc
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , size_t	  size
  , ulong	  count
  );

void *allocpolicy_realloc
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , void       *  mem
  , size_t	  size
  );

void *allocpolicy_recalloc
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , void       *  mem
  , size_t	  size
  , ulong	  count
  );

void allocpolicy_free
  ( AllocPolicy * pol
  , char const *  file
  , char const *  func
  , long	  line
  , void       *  mem
  );

void *allocpolicy_walk
  ( AllocPolicy *   pol
  , void *	    walkinfo
  , PoolVisitProc * entry
  );

// Some convenience functions

int allocpolicy_vasprintf
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , char **       bufp
  , const char *  format
  , va_list       args
  )
GCC_PRINTF(7,0);

int allocpolicy_asprintf
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , char **       bufp
  , const char *  format
  , ...
  )
GCC_PRINTF(7,8);

char *allocpolicy_strdup
  ( AllocPolicy * pol
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

extern LogPolicy   const logPolicy_default;
extern AllocPolicy const allocPolicy_default;

#endif
