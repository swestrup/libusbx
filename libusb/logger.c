/* -*- Mode: C; indent-tabs-mode:t ; c-basic-offset:8 -*- */
/*
 * Allocation functions for libusbx
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


#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "loggeri.h"

#undef printf
#undef fprintf
#undef vprintf
#undef vfprintf

/** \ingroup log
 * Return a text string corresponding to a given log level.
 *
 * This is a convenience function that returns a string equivalent to the name
 * of the current log level, or "unknown" in the case of an invalid level.
 *
 * \param[in] level log level to return a string for.
 */
char const * libusb_log_level_str(libusb_log_level level)
{
	static char const * level_str[LIBUSB_LOG_LEVEL_TRACE+1] = {
		[LIBUSB_LOG_LEVEL_NONE]    = "none",
		[LIBUSB_LOG_LEVEL_ERROR]   = "error",
		[LIBUSB_LOG_LEVEL_WARNING] = "warning",
		[LIBUSB_LOG_LEVEL_INFO]    = "info",
		[LIBUSB_LOG_LEVEL_DEBUG]   = "debug",
		[LIBUSB_LOG_LEVEL_TRACE]   = "trace"
	};

	if( level < LIBUSB_LOG_LEVEL_NONE || level > LIBUSB_LOG_LEVEL_TRACE )
		return "unknown";
	else
		return level_str[level];
}

/** \ingroup log
 * Set log message verbosity.
 *
 * The default level is LIBUSB_LOG_LEVEL_NONE, which means no messages are
 * ever printed. If you choose to increase the message verbosity level, ensure
 * that your application does not close the stdout/stderr file descriptors.
 *
 * You are advised to use level LIBUSB_LOG_LEVEL_WARNING. libusbx is conservative
 * with its message logging and most of the time, will only log messages that
 * explain error conditions and other oddities. This will help you debug
 * your software.
 *
 * If the LIBUSB_DEBUG environment variable was set when libusbx was
 * initialized, this function does nothing: the message verbosity is fixed
 * to the value in the environment variable.
 *
 * If libusbx was compiled without any message logging, this function does
 * nothing: you'll never get any messages.
 *
 * If libusbx was compiled with verbose debug message logging, this function
 * does nothing: you'll always get messages from all levels.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param level debug level to set
 */
void API_EXPORTED libusb_set_debug(libusb_context *ctx, int level)
{
	USBI_GET_CONTEXT(ctx);
	if (!ctx->debug_fixed)
		usbi_log_set_level(ctx,level);
}

/* By default libusb uses a simple message logger that prints informational
 * messages to stdout and debug, warning and error messages to stderr.
 *
 * this function can be used to set the logger for a particular libusb context
 * to one of the user's choosing.
 *
 * \param ctx the context to operate on, or NULL for the default context
 * \param logger the alternate logger to register for this context
 */
void API_EXPORTED libusb_set_logger(libusb_context *ctx, libusb_logger *logger)
{
	USBI_GET_CONTEXT(ctx);
	usbi_logger_exit(ctx);
	ctx->logger = logger;
	usbi_logger_init(ctx);
}

/* This function can be used to determine which logger is currently set on a
 * given context.
 */
DEFAULT_VISIBILITY libusb_logger * LIBUSB_CALL libusb_get_logger(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	return ctx->logger;
}



/* Android-specific default logger */
#ifdef __ANDROID__

#define USBI_ANDROID_BUFFER_SIZE USBI_MAX_LOG_LEN

/* This is a variant of snprintf that is guaranteed to not output more than
   *len characters of output, including a trailing '\0'. It returns a boolean
   indicating whether the output had to be truncated. The first two parameters
   are pointers to a buffer pointer, and a pointer to the size remaining in
   the buffer. Both are updated to reflect the number of non-null characters
   written to the buffer. That is, after storing "abc\0" the buffer pointer
   will be updated to point to the final "\0" and the size will be decreased
   by 3.
*/

static int usbi_bufprintf(
	char 		** out,
	size_t		*  len,
	char const	*  fmt,
	va_list		   args
)
{
	int ret = false;

	if(!*len)
		return true;

	int n = vsnprintf(*out, *len, fmt, args);

	if( n < 0 )	  /* output error? How? */
		n = 0;
	else if( (unsigned)n >= *len ) { /* did we truncate? */
		ret = true;
		n = (*len)-1;
	}
	*out += n;
	*len -= n;
	return ret;
}

typedef struct usbi_android_logger_log {
	usbi_mutex_static_t	mutex;
	libusb_log_level	level;
	int			started;
	int		   	prio;
	char			buffer[USBI_ANDROID_BUFFER_SIZE];
	size_t			space;
	char		      * ptr;
} usbi_android_logger_log;

static usbi_android_logger_log android_log = {
	.mutex   = USBI_MUTEX_INITIALIZER,
	.level   = LIBUSB_LOG_LEVEL_NONE,
	.started = false,
	.prio    = 0,
	.space   = USBI_ANDROID_BUFFER_SIZE,
	.ptr     = NULL
};

static void usbi_android_logger_entry_begin(
	void             * data,
	libusb_log_level   level,
	char const	 * file,
	char const	 * func,
	long	           line,
	double		   stamp
)
{
	usbi_android_logger_log * log = data;

	if( level > log->level || log->started )
		return;

	usbi_mutex_lock(&log->mutex);
	log->started   = true;
	log->space     = USBI_ANDROID_BUFFER_SIZE;
	log->buffer[0] = '\0';
	log->ptr       = &log->buffer[0];
		
	switch (level) {
	case LIBUSB_LOG_LEVEL_INFO:
		log->prio = ANDROID_LOG_INFO;
		break;
	case LIBUSB_LOG_LEVEL_WARNING:
		log->prio = ANDROID_LOG_WARN;
		break;
	case LIBUSB_LOG_LEVEL_ERROR:
		log->prio = ANDROID_LOG_ERROR;
		break;
	case LIBUSB_LOG_LEVEL_DEBUG:
	case LIBUSB_LOG_LEVEL_TRACE:
		log->prio = ANDROID_LOG_DEBUG;
		break;
	default:
		log->prio = ANDROID_LOG_UNKNOWN;
		break;
	}
}

static void usbi_android_logger_entry_extend(
	void		 * data,
	char const	 * format,
	va_list		   args
)
{	
	usbi_android_logger_log * log = data;

	if( !log->started || !log->space )
		return;

	usbi_bufprintf(&log->ptr,&log->space,format,args);
}

static void usbi_android_logger_entry_end(
	void		 * data
)
{
	usbi_android_logger_log * log = data;

	if( !log->started )
		return;

	__android_log_write(log->prio, "LibUsb", log->buffer);
	log->started = false;
	usbi_mutex_unlock(&log->mutex);
}

static libusb_log_level usbi_android_logger_get_level(
	void * data
)
{
	usbi_android_logger_log * log = data;

	return log->level;
}

static void usbi_android_logger_set_level(
	void			* data,
	libusb_log_level	  level
)
{
	usbi_android_logger_log * log = data;

	log->level = level;
}
	

static libusb_logger usbi_android_logger = {
	.data      = &android_log,
	.init      = NULL,
	.exit      = NULL,
	.begin     = usbi_android_logger_entry_begin,
	.extend    = usbi_android_logger_entry_extend,
	.end       = usbi_android_logger_entry_end,
	.get_level = usbi_android_logger_get_level,
	.set_level = usbi_android_logger_set_level
};

libusb_logger * libusb_default_logger = &usbi_android_logger;

#else

typedef struct usbi_default_logger_log {
	usbi_mutex_static_t	  mutex;
	libusb_log_level	  level;
	int			  header;
	int			  started;
	FILE		        * stream;
} usbi_default_logger_log;

static usbi_default_logger_log default_log = {
	.mutex   = USBI_MUTEX_INITIALIZER,
	.level   = LIBUSB_LOG_LEVEL_NONE,
	.header  = false,
	.started = false,
	.stream  = NULL
};

static void usbi_default_logger_init(
	void		 * data
)
{
	usbi_default_logger_log * log = data;
	log->header = false;
}

static void usbi_default_logger_header(
	usbi_default_logger_log * log
)
{
	fprintf(log->stream,"[timestamp] [threadID] facility level [function call] <message>\n");
	fprintf(log->stream,"--------------------------------------------------------------------------------\n");
	log->header = true;
}

static void usbi_default_logger_entry_begin(
	void             * data,
	libusb_log_level   level,
	char const	 * file,
	char const	 * func,
	long	           line,
	double		   stamp
)
{
	usbi_default_logger_log * log = data;

	if( level > log->level || log->started )
		return;

	usbi_mutex_lock(&log->mutex);
	log->started = true;
	log->stream  = (level <= LIBUSB_LOG_LEVEL_WARNING) ? stderr : stdout;

	char const	* prefix = libusb_log_level_str(level);
	int 		  tid    = usbi_get_tid();
	
	if( level >= LIBUSB_LOG_LEVEL_DEBUG ) {
		if( !log->header )
			usbi_default_logger_header(log);
		fprintf(log->stream, "[%9.06f] [%08x] ", stamp, tid);
	}
	fprintf(log->stream, "libusbx: %s [%s] ",prefix,func);
	
}

static void usbi_default_logger_entry_extend(
	void		 * data,
	char const	 * format,
	va_list		   args
)
{	
	usbi_default_logger_log * log = data;

	if( !log->started )
		return;

	vfprintf(log->stream,format,args);
}

static void usbi_default_logger_entry_end(
	void		 * data
)
{
	usbi_default_logger_log * log = data;

	if( !log->started )
		return;

	fprintf(log->stream,"\n");
	log->started = false;
	usbi_mutex_unlock(&log->mutex);
}

static libusb_log_level usbi_default_logger_get_level(
	void * data
)
{
	usbi_default_logger_log * log = data;

	return log->level;
}

static void usbi_default_logger_set_level(
	void			* data,
	libusb_log_level	  level
)
{
	usbi_default_logger_log * log = data;

	log->level = level;
}
	

static libusb_logger usbi_default_logger = {
	.data      = &default_log,
	.init      = usbi_default_logger_init,
	.exit      = NULL,
	.begin     = usbi_default_logger_entry_begin,
	.extend    = usbi_default_logger_entry_extend,
	.end       = usbi_default_logger_entry_end,
	.get_level = usbi_default_logger_get_level,
	.set_level = usbi_default_logger_set_level
};

libusb_logger * libusb_default_logger = &usbi_default_logger;

#endif

