#ifndef __KERNEL__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

#define TUB_DEVNAME "tub"

#ifndef __KERNEL__

/**tub_open
 * Open the driver and return a file descriptor to use with other APIs.
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int tub_open()
{
	int ret;
	
	ret = open("/dev/" TUB_DEVNAME, O_RDWR);
	if (ret < 0) ret = -errno;
	return ret;
}

/**tub_close
 * Call this API when you are done using the driver. DO NOT USE THE
 * FILE DESCRIPTOR AFTER YOU CALLED THIS API.
 * @param fd	file descriptor returned by tub_open().
 * @return	file descriptor if successful, a negative value defined 
 *		in errno.h otherwise.
 */
static inline int tub_close(int fd)
{
	int ret;
	
	ret = close(fd);
	if (ret < 0) ret = -errno;
	return ret;
}

#endif	/* #ifndef __KERNEL__ */
