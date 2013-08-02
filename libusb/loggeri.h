/*
 * Internal logger header for libusbx
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

#ifndef LOGGERI_H
#define LOGGERI_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include "libusb.h"
#include "libusbi.h"

// This header file defines functions and macros for using the abstract
// logging system inside libusb.

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

extern libusb_logger * libusb_default_logger;

// Acquire any needed locks on and/or open any required output streams, and
// output any initial header info for a log entry.
static inline void libusb_logger_entry_begin(
	libusb_logger    * logger,
	libusb_log_level   level,
	char const       * file,
	char const       * func,
	long	           line
)
{
	logger->begin(logger->data,level,file,func,line,usbi_gettimestamp());
}
	 
// Output some or all of the data for a log entry. This function can be called
// multiple times between the _begin and _end functions to output multiple
// pieces of data.
static inline void libusb_logger_entry_extend_v(
	libusb_logger * logger,
	char const    * format,
	va_list	        args
)
{
	logger->extend(logger->data,format,args);
}

// Output some or all of the data for a log entry. This function can be called
// multiple times between the _begin and _end functions to output multiple
// pieces of data. 
static inline void libusb_logger_entry_extend(
	libusb_logger * logger,
	char const    * format,
	...
)
{
	va_list args;

	va_start(args,format);
	logger->extend(logger->data,format,args);
	va_end(args);
}

// Output any trailing log entry info, and optionally flush and close streams
// and release any output serialization locks.
static inline void libusb_logger_entry_end(libusb_logger* logger)
{
	logger->end(logger->data);
}

static inline void libusb_logger_set_level(libusb_logger* logger, libusb_log_level level)
{
	logger->set_level(logger->data,level);
}	

static inline libusb_log_level libusb_logger_get_level(libusb_logger* logger)
{
	return logger->get_level(logger->data);
}	


static inline void _usbi_log_v(
	struct libusb_context * ctx,
	enum libusb_log_level   level,
	const char            * file,
	const char            * function,
	long                    line,
	const char            * format,
	va_list			args
)
{
	libusb_logger * logger = libusb_context_get_logger(ctx);

	libusb_logger_entry_begin(logger,level,file,function,line);
	libusb_logger_entry_extend_v(logger,format,args);
	libusb_logger_entry_end(logger);
}

static inline void _usbi_log(
	struct libusb_context * ctx,
	enum libusb_log_level   level,
	const char            * file,
	const char            * function,
	long                    line,
	const char            * format,
	...
)
{
	va_list args;

	va_start(args,format);
	_usbi_log_v(ctx,level,file,function,line,format,args);
	va_end(args);
}

static inline void _usbi_trc(
	struct libusb_context * ctx,
	enum libusb_log_level   level,
	const char            * file,
	const char            * function,
	long                    line
)
{
	libusb_logger * logger = libusb_context_get_logger(ctx);

	libusb_logger_entry_begin(logger,level,file,function,line);
	libusb_logger_entry_end(logger);
}

#define usbi_trc(ctx)   _usbi_trc(ctx, LIBUSB_LOG_LEVEL_TRACE,__FILE__,__FUNCTION__,__LINE_)
#define usbi_log_set_level(ctx,lvl) libusb_logger_set_level(libusb_context_get_logger(ctx),lvl)
#define usbi_log_get_level(ctx)     libusb_logger_get_level(libusb_context_get_logger(ctx))

#if !defined(_MSC_VER) || _MSC_VER >= 1400

#define usbi_log(ctx, lvl, ...) _usbi_log(ctx,lvl,__FILE__,__FUNCTION__,__LINE__,__VA_ARGS__)

#define usbi_err(ctx, ...)  usbi_log(ctx, LIBUSB_LOG_LEVEL_ERROR,   __VA_ARGS__)
#define usbi_warn(ctx, ...) usbi_log(ctx, LIBUSB_LOG_LEVEL_WARNING, __VA_ARGS__)
#define usbi_dbg(ctx, ...)  usbi_log(ctx, LIBUSB_LOG_LEVEL_DEBUG,   __VA_ARGS__)
#define usbi_info(ctx, ...) usbi_log(ctx, LIBUSB_LOG_LEVEL_INFO,    __VA_ARGS__)

#else /* !defined(_MSC_VER) || _MSC_VER >= 1400 */

#ifdef ENABLE_LOGGING
#define USBI_MSC_LOG_BODY(ctx,fmt,lvl)	{				\
		va_list args;						\
		va_start(args, fmt);					\
		usbi_log_v(ctx, lvl, NULL, NULL, 0 , fmt, args);	\
		va_end(args);						\
}
#else
#define USBI_MSC_LOG_BODY(ctx,fmt,lvl)  {	\
		(void)ctx;			\
                (void)fmt;			\
}
#endif


static inline void usbi_err(struct libusb_context *ctx, const char *format,...)
	USBI_MSC_LOG_BODY(ctx,LIBUSB_LOG_LEVEL_ERROR)

static inline void usbi_warn(struct libusb_context *ctx, const char *format,...)
	USBI_MSC_LOG_BODY(ctx,LIBUSB_LOG_LEVEL_WARNING)

static inline void usbi_info(struct libusb_context *ctx, const char *format,...)
	USBI_MSC_LOG_BODY(ctx,LIBUSB_LOG_LEVEL_INFO)

static inline void usbi_dbg(struct libusb_context *ctx, const char *format, ...)
	USBI_MSC_LOG_BODY(ctx,LIBUSB_LOG_LEVEL_DEBUG)

#endif /* !defined(_MSC_VER) || _MSC_VER >= 1400 */

#endif
