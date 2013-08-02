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

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

// Define an abstract allocator, for those who want to replace libusb's
// memory allocator with their own

/** \ingroup alloc
 *
 * The type of a user-provided allocation function. This one function acts as
 * a replacement for malloc, calloc, realloc and free, as well as taking
 * additional arguments indicating the file, line and function in which the
 * memory request originated, plus a timestamp and an arbitrary label. These
 * additional arguments are intended to aid in debugging and the allocation
 * function is free to use or ignore them as it wishes.
 *
 * \param[in] pool a user-defined memory pool or other allocation context
 * \param[in] label arbitrary label (often a type name) of this memory
 * \param[in] file file name in which allocation request originated
 * \param[in] func function name in which allocation request originated
 * \param[in] line line number in file from which allocation request originated
 * \param[in] stamp timestamp (seconds) since libusbx context was initialized.
 * \param[in] mem existing memory to resize or free (or NULL)
 * \param[in] head size of header to allocate
 * \param[in] count number of contiguous memory units to allocate after header
 * \param[in] size size of each memory unit to allocate after header
 * \returns pointer to newly allocated/realloced memory, or NULL on failure or
 *            as a result of a (successful) free operation.
 *
 * The most complex memory arrangement that this routine supports managing as
 * a single call is one where a memory region has a header of fixed size,
 * followed by a variable-sized area of one-or-more repeating units. This
 * memory format is relatively common in C code, where a programmer may wish
 * to allocate a fixed structure plus a private data area, or may wish to have
 * a structure followed by a variable-length array.
 *
 * To allow for succinct expression of these memory layouts, as well as to
 * enable user-provided allocators to perform more informed error checking,
 * there are three input parameters for describing the size of the memory
 * region being manipulated: hdr, count and size. The total size of the memory
 * area being manipulated is:
 *
 *     total = head + count * size
 *
 * Naturally the author of an allocator is free to ignore these subtleties and
 * simply record and manipulate the total size of a region.
 * 
 * The allocate function provided by an allocator author must provide several
 * features. In general, these are the features provided by the standard GCC
 * implementation of realloc:
 *
 * - Allocation:
 *
 *       If the request size is zero and input mem pointer is NULL, then a
 *       null operation is requested. The function should do no memory
 *       management, and should return NULL.
 *
 *       If the request size is non-zero and the input mem pointer is NULL,
 *       then the function should allocate a new memory area of at least size
 *       bytes, and return a pointer to it on success, or a NULL on
 *       failure. The memory area returned need not be initialized.
 *
 * - Reallocation:
 *
 *       If the request size is non-zero and the input mem pointer is
 *       non-null, then a memory reallocation/resize operation is
 *       requested. The given memory area can either be resized in-place to
 *       the newly requested size, or a new memory area of the requested size
 *       can be allocated, the old memory copied into the new area as well as
 *       fits, and the old memory area freed. In either case, a pointer to the
 *       resized area should be returned on success or NULL on failure. If
 *       returning NULL, the old memory area must be left valid for further
 *       use.
 *
 *       Note that if the memory area has been reduced in size, then data
 *       should be missing from the end of the area. Similarly, if the memory
 *       area has increased in size then new uninitialized memory will exist
 *       at the end of the area.
 *
 * - Freeing:
 *
 *	 If the request size is zero and the mem pointer is non-null then a
 *	 memory free operation is requested. The pointed-to memory should be
 *	 freed and returned to the memory pool. NULL should be returned on
 *	 success. This allows for convenient uses like:
 *
 *	 > foo.ptr = allocator_free(foo.ptr);
 *
 *	 to invalidate a pointer while freeing the memory it points to.
 */
typedef void * libusb_allocator_allocate_fn(
	void		* pool,
	char const	* label,
	char const	* file,
	char const	* func,
	long	          line,
	double	          stamp,
	void		* mem,
	size_t            head,
	unsigned long	  count,
	size_t		  size
);

/** \ingroup alloc
 *
 * The type of a user-provided memory visitation function. A function of this
 * type may be passed to a memory allocator and (if the walk function was
 * defined) the function will be called on each currently allocated memory
 * region, with as many informational parameters filled out as is possible.
 *
 * The memory visitation function may perform whatever operation it wishes,
 * although if it does anything to modify the memory pool that it is visiting,
 * the results are undefined. Typical uses of the visitation function are to
 * determine the amount of memory allocated by a pool, or to print a list of
 * all un-freed memory blocks at some point in the program.
 *
 * The walkinfo pointer that is passed to the walk function will be passed in
 * as the walkinfo for the first invocation of the visit function. Subsequent
 * calls to the visit function will be passed in the return result of the
 * previous visitation call as its walkinfo. The final output of the walk
 * function will be the return result of the final call to the visitation
 * function. This allows for data to be accumulated and passed on during the
 * operation of the visitation function.
 *
 * \param[in] walkinfo the previous output of the last call to this function.
 * \param[in] label arbitrary label (often a type name) of this memory
 * \param[in] file file name in which allocation request originated
 * \param[in] func function name in which allocation request originated
 * \param[in] line line number in file from which allocation request originated
 * \param[in] stamp timestamp (seconds) since libusbx context was initialized.
 * \param[in] mem address of the memory block
 * \param[in] size size of a memory unit in this allocated block
 * \param[in] count number of contiguous memory units allocated in this block
 * \returns an arbitrary pointer that will be passed into the next call of
 * this function.
 *
 */
typedef void * libusb_allocator_visit_fn(
	void            * walkinfo,
	char const      * label,
	char const      * file,
	char const      * func,
	long	          line,
	double            stamp,
	void		* mem,
	size_t	          head,
	unsigned long	  count,
	size_t            size
);

/** \ingroup alloc
 *
 * The type of a user-provided memory walk function. This function, if
 * provided by the user, is intended to 'walk' the data structure of the
 * currently allocated memory, and call the passed-in function on every
 * currently allocated block, providing as many or few of the informational
 * parameters as is convenient.
 *
 * The provided walkinfo pointer will be passed to the first invocation of the
 * provided visitation function, and all subsequent calls to the visitiation
 * function will be provided with the walkinfo pointer returned from the
 * previous invocation. The final returned result will be the result of last
 * call to the visitation function. Whether or not this final output is NULL
 * is of meaning only in the context of whatever it was the visitation
 * function was doing.
 *
 * \param[in] pool a user-defined memory pool or other allocation context
 * \param[in] walkinfo the initial walkinfo to pass to the visit function.
 * \param[in] visit a visitation function to call on each memory block.
 * \returns an arbitrary pointer that was returned from the last call to the
 * visit function.
 *
 */
typedef void * libusb_allocator_walk_fn(
	void		      		* pool,
	void		      		* walkinfo,
	libusb_allocator_visit_fn	* visit
);


/** \ingroup alloc
 * A structure representing an allocator that will be used by libusbx for all
 * memory allocation, reallocations and freeing within the program. Allocators
 * can be set on a context-by-context basis.
 *
 * An allocator structure should be created by whatever means the user
 * desires, but if created on the stack it should exist for the duration of
 * the use of the context it is attached to.
 *
 * The pool and walk members of this structure may be set to NULL if
 * desired. The only required member is the pointer to the allocate function.
 */
typedef struct libusb_allocator {
    /** Pointer to a memory pool or other arbitrary memory context */
    void			 * pool;

    /** The user-provided allocate function */
    libusb_allocator_allocate_fn * allocate;

    /** An optional memory inspection function */
    libusb_allocator_walk_fn     * walk;
} libusb_allocator;


libusb_allocator * LIBUSB_CALL libusb_get_allocator(libusb_context *ctx);


#endif
