/* kmemory.h
 * Include file for kmemory module which
 * allow user space application
 * to allocate and use memory areas located inside the 
 * kernel memory space. The memory can then be sent to
 * other kernel subsystems using splice system call.
 * The main reason behind this module vs using vmsplice
 * is that the kernel will know when the memory area send
 * to splice can be freed thus avoiding the classic
 * memory leak associated with vmsplice.
 * 
 * Author: Remi Machet <rmachet@slac.stanford.edu>
 * 
 * 2008 (c) SLAC, Stanford University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

/* Note to kmemory users:
 * A set of API is provided bellow for user space applications. 
 * The usage model is as follow:
 * To send data from user space to a file descriptor (note that
 * usually you would not want the memcpy, but instead directly use
 * the buffer returned by the driver):
 * int send(int fd, char *buf, int size)
 * {
 *	int kfd, ret;
 *	int pfd[2];
 *	struct iovec iovec;
 *	kfd = kmemory_open();
 *	if (kfd < 0)
 *		return kfd;
 *	ret = kmemory_map_write(kfd, &iovec, 1, size);
 *	if (ret < 0) {
 *		kmemory_close();
 * 		return ret;
 *	}
 *	memcpy(iovec.iov_base, buf, size);
 *	ret = kmemory_unmap(kfd, &iovec, 1);
 *	if (ret < 0) {
 *		kmemory_close();
 * 		return ret;
 *	}
 *	if (pipe(pfd) < 0) {
 *		ret = -errno;
 *		kmemory_close();
 *		return ret;
 *	}
 *	ret = splice(kfd, NULL, pfd[1], NULL, size, SPLICE_F_MOVE);
 *	if (ret < 0) {
 *		ret = -errno;
 * 		close(pfd[0]);
 * 		close(pfd[1]);
 *		kmemory_close();
 *		return ret;
 *	}
 *	ret = splice(pfd[0], NULL, fd, NULL, size, SPLICE_F_MOVE);
 *	if (ret < 0) {
 *		ret = -errno;
 * 		close(pfd[0]);
 * 		close(pfd[1]);
 *		kmemory_close();
 *		return ret;
 *	}
 * 	close(pfd[0]);
 * 	close(pfd[1]);
 *	kmemory_close();
 * 	return 0;
 * }
 *
 * To receive data from a file descriptor (note that here we
 * arbitrarily allocate 10 iovec for receive, that number should
 * be tuned depending on the data received; we also use another
 * memcpy here to show how to manipulate iovec structures which
 * would be irrelevant in a normal application):
 * int receive(int fd, char *buf, int size)
 * {
 *	int kfd, ret, n, nbufs, count;
 *	int pfd[2];
 *	struct iovec iovect[10];
 *	
 *	kfd = kmemory_open();
 *	if (kfd < 0)
 *		return kfd;
 *	if (pipe(pfd) < 0) {
 *		ret = -errno;
 *		kmemory_close();
 *		return ret;
 *	}
 *	ret = splice(fd, NULL, pfd[1], NULL, size, SPLICE_F_MOVE);
 *	if (ret < 0) {
 *		ret = -errno;
 * 		close(pfd[0]);
 * 		close(pfd[1]);
 *		kmemory_close();
 *		return ret;
 *	}
 *	ret = splice(pfd[0], NULL, kfd, NULL, size, SPLICE_F_MOVE);
 *	if (ret < 0) {
 *		ret = -errno;
 * 		close(pfd[0]);
 * 		close(pfd[1]);
 *		kmemory_close();
 *		return ret;
 *	}
 * 	close(pfd[0]);
 * 	close(pfd[1]);
 *	ret = kmemory_map_read(kfd, iovect, 10);
 *	if (ret < 0) {
 *		kmemory_close();
 * 		return ret;
 *	}
 *	nbufs = ret;
 * 	for (count = 0, n=0; (count < size) && (n < nbufs); count+=iovect[n].iov_len, n++)
 *		memcpy(buf + count, iovect[n].iov_base, iovect[n].iov_len);
 *	ret = kmemory_unmap(kfd, iovect, nbufs);
 *	if (ret < 0) {
 *		kmemory_close();
 * 		return ret;
 *	}
 *	kmemory_close();
 * 	return 0;
 * }
 */

#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#else
#include <linux/uio.h>
#endif

#define KMEMORY_DEVNAME "kmemory"

struct kmemory_map_data {
	int nelems;
	int flags;
	struct iovec *piovec_table;
};

#define KMEMORY_MAGIC	'k'

#define KMEMORY_IOCTL_MAP	_IOW(KMEMORY_MAGIC, 0, struct iovec *)
#define KMEMORY_IOCTL_UNMAP	_IOR(KMEMORY_MAGIC, 1, struct iovec *)

#define KMEMORY_READ	1
#define KMEMORY_WRITE	2
#define KMEMORY_ALLOC	4

#ifndef __KERNEL__

/**kmemory_open
 * Open the driver and return a file descriptor to use with other APIs.
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int kmemory_open()
{
	int ret;
	
	ret = open("/dev/" KMEMORY_DEVNAME, O_RDWR);
	if (ret < 0) ret = -errno;
	return ret;
}

/**kmemory_close
 * Call this API when you are done using the kmemory driver. DO NOT USE THE
 * FILE DESCRIPTOR AFTER YOU CALLED THIS API.
 * @fd		file descriptor returned by kmemory_open().
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int kmemory_close(int fd)
{
	int ret;
	
	ret = close(fd);
	if (ret < 0) ret = -errno;
	return ret;
}

/**kmemory_map_read
 * MAP a list of buffers received by kmemory (using splice) into
 * the caller memory space. Note that it is not guaranteed that <nelems>
 * buffers will be mapped, the caller must check the return value.
 * @fd		file descriptor returned by kmemory_open().
 * @piov	list of iovec structures pointers that will be filled with
 *		the address and size of each buffer. The application must
 *		have allocated enough space for <nelems> structures.
 * @nelems	number of struct iovec elements in piov.
 * @return	the number of iovec elements initialized if successful, a 
 * 		negative value defined in errno.h otherwise.
 */
#include <stdio.h>
static inline int kmemory_map_read(int fd, struct iovec piov[], int nelems)
{	
	struct kmemory_map_data kmemory_data;

	kmemory_data.flags = KMEMORY_READ;
	kmemory_data.nelems = nelems;
	kmemory_data.piovec_table = piov;
	return ioctl(fd, KMEMORY_IOCTL_MAP, &kmemory_data);
}

/**kmemory_map_write
 * Allocate <nelems> buffers of <size> bytes into the application memory space.
 * The caller can then fill them and send them using splice to another file 
 * descriptor.
 * @fd		file descriptor returned by kmemory_open().
 * @piov	list of iovec structures pointers that will be filled with
 *		the address and size of each buffer. The application must
 *		have allocated enough space for <nelems> structures.
 * @nelems	number of struct iovec elements in piov.
 * @size	size that should have each element in piov.
 * @return	the number of iovec elements initialized if successful, a 
 * 		negative value defined in errno.h otherwise.
 */
static inline int kmemory_map_write(int fd, struct iovec piov[], int nelems, unsigned long size)
{	
	struct kmemory_map_data kmemory_data;
	int i;

	kmemory_data.flags = KMEMORY_WRITE | KMEMORY_ALLOC;
	kmemory_data.nelems = nelems;
	kmemory_data.piovec_table = piov;
	for(i=0; i < nelems; i++)
		piov[i].iov_len = size;
	return ioctl(fd, KMEMORY_IOCTL_MAP, &kmemory_data);
}

/**kmemory_map_modify
 * MAP a list of buffers received by kmemory (using splice) into
 * the caller memory space. Note that it is not guaranteed that <nelems>
 * buffers will be mapped, the caller must check the return value.
 * The caller can then modify those data and send them with splice to another
 * file descriptor.
 * @fd		file descriptor returned by kmemory_open().
 * @piov	list of iovec structures pointers that will be filled with
 *		the address and size of each buffer. The application must
 *		have allocated enough space for up to <nelems> structures.
 * @nelems	number of struct iovec elements in piov.
 * @return	the number of iovec elements initialized if successful, a 
 * 		negative value defined in errno.h otherwise.
 */
static inline int kmemory_map_modify(int fd, struct iovec piov[], int nelems)
{	
	struct kmemory_map_data kmemory_data;

	kmemory_data.flags = KMEMORY_READ | KMEMORY_WRITE;
	kmemory_data.nelems = nelems;
	kmemory_data.piovec_table = piov;
	return ioctl(fd, KMEMORY_IOCTL_MAP, &kmemory_data);
}

/**kmemory_unmap
 * Unmap a list of struct iovec that were mapped using kmemory_map_*.
 * After calling this API, those buffers CANNOT be accessed by the 
 * application anymore (they may still be accessible by kernel
 * subsystems).
 * NOTE: You must unmap a buffer AFTER sending it to another kernel
 * subsystem with splice to avoid the buffer being freed because no
 * one is using it.
 * @piov	list of iovec structure describing each buffer (as returned
 *		by kmemory_map_*).
 * @nelems	number of struct iovec elements in piov.
 * @return	the number of iovec elements unmapped if successful, a 
 * 		negative value defined in errno.h otherwise.
 */
static inline int kmemory_unmap(int fd, struct iovec piov[], int nelems)
{
	struct kmemory_map_data kmemory_data;

	kmemory_data.flags = 0;
	kmemory_data.nelems = nelems;
	kmemory_data.piovec_table = piov;
	return ioctl(fd, KMEMORY_IOCTL_UNMAP, &kmemory_data);
}

#endif	/* #ifndef KERNEL_RELEASE */
