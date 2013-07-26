/*
 * Copyright 2012, Userful Corporation
 */
#if defined(HAVE_CONFIG)
  #include "config.h"
#endif

#if !defined(_GNU_SOURCE)
  #define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <string.h>

#include "dl_policy.h"
#include "macros.h"

// AllocPolicy indirection functions.

void *allocpolicy_alloc
  ( AllocPolicy *pol
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , size_t       size
  )
  {
    return pol->alloc(pol->pool, name, file, func, line, size, 1);
  }

void *allocpolicy_calloc
  ( AllocPolicy *pol
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , size_t       size
  , ulong	 count
  )
  {
    return pol->alloc(pol->pool, name, file, func, line, size, count);
  }

void *allocpolicy_realloc
  ( AllocPolicy *pol
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , void       * mem
  , size_t       size
  )
  {
    return pol->realloc(pol->pool, name, file, func, line, mem, size, 1);
  }


void *allocpolicy_recalloc
  ( AllocPolicy *pol
  , char const * name
  , char const * file
  , char const * func
  , long	 line
  , void       * mem
  , size_t       size
  , ulong	 count
  )
  {
    return pol->realloc(pol->pool, name, file, func, line, mem, size, count);
  }

void allocpolicy_free
  ( AllocPolicy *pol
  , char const * file
  , char const * func
  , long	 line
  , void       * mem
  )
  {
    pol->free(pol->pool, file, func, line, mem);
  }

// Some additional convenience functions

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
  { va_list	 argsc;

    va_copy(argsc, args);
    int len = vsnprintf(NULL, 0, format, argsc) + 1;
    va_end(argsc);

    char *p = pol->alloc(pol->pool,name,file,func,line,sizeof(char),len);
    if (!p)
      return -1;

    *bufp = p;
    return vsnprintf(p, len, format, args);
  }

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
  { va_list       args;

    va_start(args, format);
    int ret = allocpolicy_vasprintf(pol,name,file,func,line,bufp,format,args);
    va_end(args);
    return ret;
  }

char *allocpolicy_strdup
  ( AllocPolicy * pol
  , char const *  name
  , char const *  file
  , char const *  func
  , long	  line
  , char const *  str
  )
  { char *ret = NULL;

    if( str )
      { size_t len = strlen(str);

	ret = pol->alloc(pol->pool, name,file,func,line,sizeof(char),len+1);
	if( ret )
	  strcpy(ret,str);
      }
    return ret;
  }

// LogPolicy indirection functions.

void logpolicy_beg
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  )
  {
    pol->beg(pol->data,level,flags,filename,funcname,line);
  }

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
  {
    pol->log(pol->data,level,flags,filename,funcname,line,format,args);
  }

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
  { va_list args;

    va_start(args,format);
    pol->log(pol->data,level,flags,filename,funcname,line,format,args);
    va_end(args);
  }

void logpolicy_end
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  )
  {
    pol->end(pol->data,level,flags,filename,funcname,line);
  }

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
  { va_list args;

    va_start(args,format);
    pol->beg(pol->data,level,flags,filename,funcname,line);
    pol->log(pol->data,level,flags,filename,funcname,line,format,args);
    pol->end(pol->data,level,flags,filename,funcname,line);
    va_end(args);
  }

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
  {
    pol->beg(pol->data,level,flags,filename,funcname,line);
    pol->log(pol->data,level,flags,filename,funcname,line,format,args);
    pol->end(pol->data,level,flags,filename,funcname,line);
  }

void logpolicy_trace
  ( LogPolicy *  pol
  , LogLevel     level
  , LogFlags     flags
  , char const * filename
  , char const * funcname
  , long	 line
  )
  {
    pol->beg(pol->data,level,flags,filename,funcname,line);
    pol->end(pol->data,level,flags,filename,funcname,line);
  }

