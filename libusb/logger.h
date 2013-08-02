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
 *  - LIBUSB_LOG_LEVEL_WARNING (2) : warning and error messages are logged
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

/** \ingroup log
 *
 * The type of a user-provided log-entry-begin function. This function is
 * called to mark the start of a new entry, and is provided with most of the
 * contextual information for that entry, such as the file, function and line
 * that the log message originated on. It is intended to acquire any
 * serialization locks provided by the logger, and to do whatever setup is
 * necessary to start a new log entry record.
 *
 * Note that a logger will recieve all logged messages, even those for a lower
 * log level than is currently set. This allows the logger to determine its
 * own policy on how to interpret a log level, and what exactly it means.
 *
 * Which pieces of contextual information provided are used by the logger,
 * under what circumstances, is entirely up to the logger.
 *
 * \param[in] data logger-specific context data.
 * \param[in] level The log level of the record to start.
 * \param[in] file  The filename of the source file that issues the log
 * 	      message.
 * \param[in] line  The line number in the source file where the log
 * 	      message originated.	      
 * \param[in] stamp A timestamp giving the number of seconds since libusb
 *            was started.
 */
typedef void libusb_logger_entry_begin_fn(
	void             * data,
	libusb_log_level   level,
	char const	 * file,
	char const	 * func,
	long	           line,
	double		   stamp
);

/** \ingroup log
 *
 * The type of a user-provided log-entry-data function. This function is
 * called to fill in the actual content of a log entry. It may be called
 * multiple times and the intent is that each call appends more information to
 * the same log entry. Any imbedded newlines should be taken as a request to
 * start a new line in the same log entry, and as such it is usually
 * inappropriate for the final call of this function on an entry to provide a
 * trailing newline.
 *
 * \param[in] data logger-specific context data.
 * \param[in] format printf-style format string.
 * \param[in] args  a variable argument list of data to format.
 */
typedef void libusb_logger_entry_extend_fn(
	void		 * data,
	char const	 * format,
	va_list		   args
);

/** \ingroup log
 *
 * The type of a user-provided log-entry-finalize function. This function is
 * called to terminate the current log entry. As such it is responsible for
 * attaching any final formatting (such as a trailing newline), closing a log
 * record, flushing an output stream, or releasing serialization locks.
 *
 * \param[in] data logger-specific context data.
 */
typedef void libusb_logger_entry_end_fn(
	void		 * data
);


/** \ingroup log
 *
 * The type of a user-provided function to retreive the currently set log
 * level for this logger.
 *
 * \param[in] data logger-specific context data.
 * \returns the currently set log level
 */
typedef libusb_log_level libusb_logger_get_level_fn(
	void *data
);

/** \ingroup log
 *
 * The type of a user-provided function to set the log level for this logger.
 *
 * \param[in] data logger-specific context data.
 * \param[in] level log level to set
 */
typedef void libusb_logger_set_level_fn(
	void			* data,
	libusb_log_level 	  level
);

typedef struct libusb_logger {
	/** arbitrary log control data */
	void 			      * data;

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
