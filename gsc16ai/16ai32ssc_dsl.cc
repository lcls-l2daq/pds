#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "16ai32ssc_dsl.hh"

/*
 * ai32ssc_dsl_ioctl
 */
int ai32ssc_dsl_ioctl(int fd, int request, void *arg)
{
	int	err;
	int	status;

	status	= ioctl(fd, request, arg);

	if (status == -1)
		printf("ERROR: ioctl() failure, errno = %d\n", errno);

	err	= (status == -1) ? 1 : 0;
	return(err);
}

/*
 * ai32ssc_dsl_read
 */
int ai32ssc_dsl_read(int fd, __u32 *buf, size_t samples)
{
	size_t	bytes;
	int		status;

	bytes	= samples * 4;
	status	= read(fd, buf, bytes);

	if (status == -1)
		printf("ERROR: read() failure, errno = %d\n", errno);
	else
		status	/= 4;

	return(status);
}
