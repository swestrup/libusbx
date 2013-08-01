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

#include "allocatori.h"

#undef free
#undef malloc
#undef calloc
#undef realloc
#undef strdup
#undef strndup
#undef asprintf
#undef vasprintf

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static void * usbi_default_allocator_allocate(
	void       * pool,
	char const * label,
	char const * file,
	char const * func,
	long	     line,
	double	     stamp,
	void       * mem,
	size_t       head,
	ulong	     count,
	size_t       size
)
{
	size_t       total = head + (count * size);
	void       * ret   = NULL;
        UNUSED(pool);
        UNUSED(label);
        UNUSED(file);
        UNUSED(func);
        UNUSED(line);
        UNUSED(stamp);

	if( total == 0 ) {
		if( mem )
			free( mem );
	} else if( mem )
		ret = realloc(mem, total);
	else
		ret = malloc(total);
	return ret;
}
		  
static libusb_allocator usbi_default_allocator = {
	.pool     = NULL,
	.allocate = usbi_default_allocator_allocate,
	.walk     = NULL
};

typedef struct usbi_pool_entry usbi_pool_entry;

struct usbi_pool_entry {
	usbi_pool_entry * prev;
	usbi_pool_entry * next;
	char const      * label;
	char const      * file;
	char const      * func;
	long	          line;
	double	          stamp;
	size_t            head;
	ulong	          count;
	size_t            size;
};


typedef struct usbi_pool_control {
	usbi_pool_entry * first;
	usbi_pool_entry * last;
} usbi_pool_control;


static void usbi_pool_link( usbi_pool_control * pool,
	usbi_pool_entry *entry
)
{
	entry->next = NULL;
	entry->prev = pool->last;
	pool->last  = entry;
	if( !pool->first )
		pool->first = entry;
}

static void usbi_pool_unlink( usbi_pool_control * pool,
	usbi_pool_entry *entry
)
{
	if( entry->prev )
		entry->prev->next = entry->next;
	else
		pool->first = entry->next;
	if( entry->next )
		entry->next->prev = entry->prev;
	else
		pool->last = entry->prev;
	entry->prev = NULL;
	entry->next = NULL;
}


/* this adjusts the linked list when our address changes. Our entry's pointers
   to the list are still good, but we have to modify everyone else's pointers
   to us.
*/  
static void usbi_pool_relink( usbi_pool_control * pool,
	usbi_pool_entry *entry
)
{
	if( entry->prev )
		entry->prev->next = entry;
	else
		pool->first = entry;
	if( entry->next )
		entry->next->prev = entry;
	else
		pool->last = entry;
}



#define USBI_MEM_TO_ENTRY(mem)						\
	((usbi_pool_entry *)(((char *)(mem))-sizeof(usbi_pool_entry)))

#define USBI_ENTRY_TO_MEM(entry)					\
	((void *)(((char *)(entry))+sizeof(usbi_pool_entry)))

static void * usbi_debug_allocator_allocate(
	void       * arg,
	char const * label,
	char const * file,
	char const * func,
	long	     line,
	double	     stamp,
	void       * mem,
	size_t       head,
	ulong	     count,
	size_t       size
)
{       usbi_pool_control * pool = arg;
	size_t total = head + (count * size) + sizeof(usbi_pool_entry);
	void * ret   = NULL;

	if( total == 0 ) {
		if( mem ) {
			usbi_pool_entry * ptr = USBI_MEM_TO_ENTRY(mem);
			usbi_pool_unlink(pool,ptr);
			free( ptr );
		}
			
	} else if( mem ) {
		usbi_pool_entry * ptr = USBI_MEM_TO_ENTRY(mem);
		ret = realloc(ptr, total);
		if( ret ) {  
			if( ret != ptr ) {
				ptr = ret;
				usbi_pool_relink(pool,ptr);
			}	
			ret = USBI_ENTRY_TO_MEM(ptr);
		}	
	} else {
		ret = malloc(total);
		if( ret ) {
			usbi_pool_entry * ptr = ret;
			ptr->label = label;
			ptr->file  = file;
			ptr->func  = func;
			ptr->line  = line;
			ptr->stamp = stamp;
			ptr->head  = head;
			ptr->count = count;
			ptr->size  = size;
			usbi_pool_link(pool,ptr);
			ret = USBI_ENTRY_TO_MEM(ptr);
		}
	}
	return ret;
}

static void * usbi_debug_allocator_walk(
	void		          * arg,
	void		          * walkinfo,
	libusb_allocator_visit_fn * visit
)
{
	usbi_pool_control * pool = arg;  
	usbi_pool_entry * entry = pool->first;

	while( entry ) {
		walkinfo = visit(walkinfo, entry->label, entry->file,
			entry->func, entry->line, entry->stamp,
			USBI_ENTRY_TO_MEM(entry), entry->head, entry->count,
			entry->size);
		entry = entry->next;
	}
	return walkinfo;
}

static usbi_pool_control usbi_debug_allocator_pool = {
	.first = NULL,
	.last  = NULL
};

static libusb_allocator usbi_debug_allocator = {
	.pool     = &usbi_debug_allocator_pool,
	.allocate = usbi_debug_allocator_allocate,
	.walk     = usbi_debug_allocator_walk
};


libusb_allocator * libusb_debug_allocator = &usbi_debug_allocator;
libusb_allocator * libusb_default_allocator = &usbi_default_allocator;
