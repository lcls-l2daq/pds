#include <linux/ioctl.h>
#ifndef __KERNEL__
#include <stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#define DMA_SPLICE_DEVNAME "dma_splice"
#define DMA_SPLICE_MAGIC 'd'
#define DMA_SPLICE_IOCTL_INIT	_IOW(DMA_SPLICE_MAGIC, 0, void*)
#define DMA_SPLICE_IOCTL_QUEUE	_IOW(DMA_SPLICE_MAGIC, 1, void*)
#define DMA_SPLICE_IOCTL_SKIP	_IOW(DMA_SPLICE_MAGIC, 2, void*)
#define DMA_SPLICE_IOCTL_NOTIFY	_IOW(DMA_SPLICE_MAGIC, 3, void*)

struct dma_splice_ioctl_desc {
  unsigned long addr;
  unsigned long end;
  unsigned long arg;
};


#ifndef __KERNEL__

/**dma_splice_open
 * Open the driver and return a file descriptor to use with other APIs.
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int dma_splice_open()
{
	int ret;
	
	ret = open("/dev/" DMA_SPLICE_DEVNAME, O_RDWR);
	if (ret < 0) ret = -errno;
	return ret;
}

/**dma_splice_close
 * Call this API when you are done using the driver. DO NOT USE THE
 * FILE DESCRIPTOR AFTER YOU CALLED THIS API.
 * @param fd	file descriptor returned by dma_splice_open().
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int dma_splice_close(int fd)
{
	int ret;
	
	ret = close(fd);
	if (ret < 0) ret = -errno;
	return ret;
}

static inline int dma_splice_initialize(int fd, void *base, int len)
{
        struct dma_splice_ioctl_desc desc;
	desc.addr = (unsigned long)base;
	desc.end  = (unsigned long)base+len;
	return ioctl(fd, DMA_SPLICE_IOCTL_INIT, &desc);
}

static inline int dma_splice_queue(int fd, void *base, int len, unsigned long release_arg)
{
        struct dma_splice_ioctl_desc desc;
	desc.addr = (unsigned long)base;
	desc.end  = (unsigned long)base+len;
	desc.arg  = release_arg;
	return ioctl(fd, DMA_SPLICE_IOCTL_QUEUE, &desc);
}

static inline int dma_splice_skip(int fd, unsigned long len)
{
	return ioctl(fd, DMA_SPLICE_IOCTL_SKIP, len);
}

static inline int dma_splice_notify(int fd, unsigned long* release_arg)
{
	return ioctl(fd, DMA_SPLICE_IOCTL_NOTIFY, release_arg);
}

#endif	/* #ifndef __KERNEL__ */
