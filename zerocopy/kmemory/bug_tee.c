#define _GNU_SOURCE
#include "kmemory.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define READ_SIZE	2124

int main(int argc, char *argv[])
{
	int fdKmemoryDrv, fdFileIn, fdFileOut, ret, i;
	int fdPipe1[2], fdPipe2[2];
	int toread = READ_SIZE;
	struct iovec iov;
	unsigned int buf[READ_SIZE];
	unsigned int bufkmem[READ_SIZE];

	fdKmemoryDrv = kmemory_open();
	if(fdKmemoryDrv < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n" \
				"Are you sure the kmemory driver is loaded ?\n\n",
				"/dev/" KMEMORY_DEVNAME, strerror(errno));
		return -errno;
	}
	/* Open the file to read/write from */
	fdFileIn = open(argv[1], O_RDONLY | O_NONBLOCK);
	if(fdFileIn < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n", argv[1], strerror(errno));
		return -errno;
	}
	if (argc > 2) {
		fdFileOut = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
		if(fdFileOut < 0) {
			fprintf(stderr, "ERROR: Could not open %s, %s.\n", argv[2], strerror(errno));
			return -errno;
		}
	} else fdFileOut = -1;
	/* First we open input and output pipe */
	if(pipe(fdPipe1) < 0) {
		fprintf(stderr, "ERROR: pipe failed, error %s.\n", strerror(errno));
		return -errno;
	}
	if(pipe(fdPipe2) < 0) {
		fprintf(stderr, "ERROR: pipe failed, error %s.\n", strerror(errno));
		return -errno;
	}
	ret = splice(fdFileIn, NULL, fdPipe1[1], NULL, toread, SPLICE_F_MOVE);
	if (ret < 0) {
		fprintf(stderr, "ERROR: splice(fdIn->pipe1) failed, error %s.\n", strerror(errno));
		return -errno;
	} else if (ret != toread)
		fprintf(stderr, "WARNING: splice(fdIn->pipe1) processed %dB instead of %dB.\n",ret,toread);
	toread = ret;
	ret = tee(fdPipe1[0],fdPipe2[1],toread,SPLICE_F_MOVE);
	if (ret < 0) {
		fprintf(stderr, "ERROR: tee failed, error %s.\n", strerror(errno));
		return -errno;
	} else if (ret != toread)
		fprintf(stderr, "WARNING: tee processed %dB instead of %dB.\n",ret,toread);
	ret = splice(fdPipe2[0], NULL, fdKmemoryDrv, NULL, toread, SPLICE_F_MOVE);
	if (ret < 0) {
		fprintf(stderr, "ERROR: splice(pipe2->kmemory) failed, error %s.\n", strerror(errno));
		return -errno;
	} else if (ret != toread)
		fprintf(stderr, "WARNING: splice(pipe2->kmemory) processed %dB instead of %dB.\n",ret,toread);
	ret = kmemory_map_read(fdKmemoryDrv, &iov, 1);
	if (ret < 0) {
		fprintf(stderr, "ERROR: kmemory_map_read failed, error %s.\n", strerror(errno));
		return -errno;
	} else if (ret != 1) {
		fprintf(stderr, "ERROR: kmemory_map_read returned %d.\n", ret);
		return -EIO;
	} else if (iov.iov_len != toread)
		fprintf(stderr, "WARNING: kmemory_map_read processed %dB instead of %dB.\n",iov.iov_len,toread);
	toread = iov.iov_len;
	memcpy(bufkmem,iov.iov_base,toread);
	ret = kmemory_unmap(fdKmemoryDrv, &iov, 1);
	if (ret < 0) {
		fprintf(stderr, "ERROR: kmemory_unmap failed, error %s.\n", strerror(errno));
		return -errno;
	} else if (ret != 1) {
		fprintf(stderr, "ERROR: kmemory_unmap returned %d.\n", ret);
		return -EIO;
	};
	if (fdFileOut < 0) {
		ret = read(fdPipe1[0], buf, toread);
		if (ret < 0) {
			fprintf(stderr, "ERROR: read failed, error %s.\n", strerror(errno));
			return -errno;
		} else if (ret != toread)
			fprintf(stderr, "WARNING: read processed %dB instead of %dB.\n",ret,toread);
		for(i=0; i<toread>>2; i++) {
			if(buf[i] != bufkmem[i])
				printf("@%d: %04x != %04x\n",i,buf[i],bufkmem[i]);
		}
		printf("Compare done.\n");
	} else {
		ret = splice(fdPipe1[0], NULL, fdFileOut, NULL, toread, SPLICE_F_MOVE);
		if (ret < 0) {
			fprintf(stderr, "ERROR: splice(pipe1->out) failed, error %s.\n", strerror(errno));
			return -errno;
		} else if (ret != toread)
			fprintf(stderr, "WARNING: splice(pipe1->out) processed %dB instead of %dB.\n",ret,toread);
	}
	return 0;
}
