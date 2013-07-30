/*
 * Allocator support for libusbx
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

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

/** \ingroup log
 *  Log message levels.
 *  - LIBUSB_LOG_LEVEL_NONE    (0) : no messages are ever logged
 *  - LIBUSB_LOG_LEVEL_ERROR   (1) : error messages are logged
 *  - LIBUSB_LOG_LEVEL_WARNING (2) : warning, and error messages are logged
 *  - LIBUSB_LOG_LEVEL_INFO    (3) : as (2) but includes informational messages 
 *  - LIBUSB_LOG_LEVEL_DEBUG   (4) : as (3) but includes debug messages
 *  - LIBUSB_LOG_LEVEL_TRACE   (5) : as (4) but includes internal trace messages */
typedef enum libusb_log_level {
	LIBUSB_LOG_LEVEL_NONE = 0,
	LIBUSB_LOG_LEVEL_ERROR,
	LIBUSB_LOG_LEVEL_WARNING,
	LIBUSB_LOG_LEVEL_INFO,
	LIBUSB_LOG_LEVEL_DEBUG,
	LIBUSB_LOG_LEVEL_TRACE
} libusb_log_level;

char const * usbi_log_level_str(libusb_log_level level);

typedef void libusb_logger_entry_begin_fn(
	void             * data,
	libusb_log_level   level,
	char const	 * file,
	char const	 * func,
	long	           line,
	double		   stamp
);

typedef void libusb_logger_entry_extend_fn(
	void		 * data,
	char const	 * format,
	va_list		   args
);

typedef void libusb_logger_entry_end_fn(
	void		 * data
);

typedef libusb_log_level libusb_logger_get_level_fn(void *data);
typedef void libusb_logger_set_level_fn(void *data, libusb_log_level level);

typedef struct libusb_logger {
	/** arbitrary log control data */
	void *	      data;

        /** Start a new log entry */
	libusb_logger_entry_begin_fn  * begin;

        /** Extend the current log entry with (more) data */
        libusb_logger_entry_extend_fn * extend;	    

        /** Finish the log entry */
	libusb_logger_entry_end_fn    * end;

	/** get the current log level */
        libusb_logger_get_level_fn    * get_level;

	/** set the current log level */
	libusb_logger_set_level_fn    * set_level;
} libusb_logger;




#endif
