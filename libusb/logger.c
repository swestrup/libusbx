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

#include "loggeri.h"

#undef printf
#undef fprintf

#include <stdio.h>
#include <string.h>



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
		usbi_set_log_level(ctx,level);
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
	ctx->logger = logger;
}

/* This function can be used to set determine which logger is currently set on
 * a given context.
 */
DEFAULT_VISIBILITY libusb_logger * API_EXPORTED libusb_get_logger(libusb_context *ctx)
{
	USBI_GET_CONTEXT(ctx);
	return ctx->logger;
}


void usbi_default_logger(libusb_context *ctx, void *logdata,
	double stamp, int level, char const *prefix, char const * function,
	char const * format, va_list args)
{
	FILE *stream = (level == LIBUSB_LOG_LEVEL_INFO) ? stdout : stderr;

	fprintf(stream, "libusb: %12.6f %s [%s] ", stamp, prefix, function);

	vfprintf(stream, format, args);

	fprintf(stream, "\n");
}

void usbi_log_v(struct libusb_context *ctx, enum libusb_log_level level,
	const char *function, const char *format, va_list args)
{
	const char *prefix;
	struct timeval now;
	static struct timeval first = { 0, 0 };

	USBI_GET_CONTEXT(ctx);
	if (ctx->debug < 4-level)
		return;

	usbi_gettimeofday(&now, NULL);
	if (!first.tv_sec) {
		first.tv_sec = now.tv_sec;
		first.tv_usec = now.tv_usec;
	}
	if (now.tv_usec < first.tv_usec) {
		now.tv_sec--;
		now.tv_usec += 1000000;
	}
	now.tv_sec -= first.tv_sec;
	now.tv_usec -= first.tv_usec;

	double stamp = (double)now.tv_sec + (((double)now.tv_usec)/1000000.0);

	switch (level) {
	case LIBUSB_LOG_LEVEL_INFO:
		prefix = "info";
		break;
	case LIBUSB_LOG_LEVEL_WARNING:
		prefix = "warning";
		break;
	case LIBUSB_LOG_LEVEL_ERROR:
		prefix = "error";
		break;
	case LIBUSB_LOG_LEVEL_DEBUG:
		prefix = "debug";
		break;
	default:
		prefix = "unknown";
		break;
	}

	ctx->logger(ctx, ctx->logdata, stamp, level, prefix, function,
		format, args);
}

void usbi_log(struct libusb_context *ctx, usbi_log_level level,
	const char *function, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	usbi_log_v(ctx, level, function, format, args);
	va_end (args);
}

static void usbi_log_str(struct libusb_context *ctx, const char * str)
{
	UNUSED(ctx);
	fputs(str, stderr);
}

void usbi_log_v(struct libusb_context *ctx, enum libusb_log_level level,
	const char *function, const char *format, va_list args)
{
	const char *prefix = "";
	char buf[USBI_MAX_LOG_LEN];
	struct timeval now;
	int global_debug, header_len, text_len;
	static int has_debug_header_been_displayed = 0;

#ifdef ENABLE_DEBUG_LOGGING
	global_debug = 1;
	UNUSED(ctx);
#else
	USBI_GET_CONTEXT(ctx);
	if (ctx == NULL)
		return;
	global_debug = (ctx->debug == LIBUSB_LOG_LEVEL_DEBUG);
	if (!ctx->debug)
		return;
	if (level == LIBUSB_LOG_LEVEL_WARNING && ctx->debug < LIBUSB_LOG_LEVEL_WARNING)
		return;
	if (level == LIBUSB_LOG_LEVEL_INFO && ctx->debug < LIBUSB_LOG_LEVEL_INFO)
		return;
	if (level == LIBUSB_LOG_LEVEL_DEBUG && ctx->debug < LIBUSB_LOG_LEVEL_DEBUG)
		return;
#endif

#ifdef __ANDROID__
	int prio;
	switch (level) {
	case LOG_LEVEL_INFO:
		prio = ANDROID_LOG_INFO;
		break;
	case LOG_LEVEL_WARNING:
		prio = ANDROID_LOG_WARN;
		break;
	case LOG_LEVEL_ERROR:
		prio = ANDROID_LOG_ERROR;
		break;
	case LOG_LEVEL_DEBUG:
		prio = ANDROID_LOG_DEBUG;
		break;
	default:
		prio = ANDROID_LOG_UNKNOWN;
		break;
	}

	__android_log_vprint(prio, "LibUsb", format, args);
#else
	usbi_gettimeofday(&now, NULL);
	if ((global_debug) && (!has_debug_header_been_displayed)) {
		has_debug_header_been_displayed = 1;
		usbi_log_str(ctx, "[timestamp] [threadID] facility level [function call] <message>\n");
		usbi_log_str(ctx, "--------------------------------------------------------------------------------\n");
	}
	if (now.tv_usec < timestamp_origin.tv_usec) {
		now.tv_sec--;
		now.tv_usec += 1000000;
	}
	now.tv_sec -= timestamp_origin.tv_sec;
	now.tv_usec -= timestamp_origin.tv_usec;

	switch (level) {
	case LIBUSB_LOG_LEVEL_INFO:
		prefix = "info";
		break;
	case LIBUSB_LOG_LEVEL_WARNING:
		prefix = "warning";
		break;
	case LIBUSB_LOG_LEVEL_ERROR:
		prefix = "error";
		break;
	case LIBUSB_LOG_LEVEL_DEBUG:
		prefix = "debug";
		break;
	case LIBUSB_LOG_LEVEL_NONE:
		break;
	default:
		prefix = "unknown";
		break;
	}

	if (global_debug) {
		header_len = snprintf(buf, sizeof(buf),
			"[%2d.%06d] [%08x] libusbx: %s [%s] ",
			(int)now.tv_sec, (int)now.tv_usec, usbi_get_tid(), prefix, function);
	} else {
		header_len = snprintf(buf, sizeof(buf),
			"libusbx: %s [%s] ", prefix, function);
	}

	if (header_len < 0 || header_len >= sizeof(buf)) {
		/* Somehow snprintf failed to write to the buffer,
		 * remove the header so something useful is output. */
		header_len = 0;
	}
	/* Make sure buffer is NUL terminated */
	buf[header_len] = '\0';
	text_len = vsnprintf(buf + header_len, sizeof(buf) - header_len,
		format, args);
	if (text_len < 0 || text_len + header_len >= sizeof(buf)) {
		/* Truncated log output. On some platforms a -1 return value means
		 * that the output was truncated. */
		text_len = sizeof(buf) - header_len;
	}
	if (header_len + text_len + sizeof(USBI_LOG_LINE_END) >= sizeof(buf)) {
		/* Need to truncate the text slightly to fit on the terminator. */
		text_len -= (header_len + text_len + sizeof(USBI_LOG_LINE_END)) - sizeof(buf);
	}
	strcpy(buf + header_len + text_len, USBI_LOG_LINE_END);

	usbi_log_str(ctx, buf);
#endif
}

void usbi_log(struct libusb_context *ctx, enum libusb_log_level level,
	const char *function, const char *format, ...)
{
	va_list args;

	va_start (args, format);
	usbi_log_v(ctx, level, function, format, args);
	va_end (args);
}

