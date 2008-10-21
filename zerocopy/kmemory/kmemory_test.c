/* kmemory_test.c
 * Little program that make use of the kmemory driver to read
 * and write to a file.
 * kmemory_test <file> behaves like cat
 * kmemory_test -w <file> read from stdin and write to <file>
 * 
 * Author: Remi Machet <rmachet@slac.stanford.edu>
 * 
 * 2008 (c) SLAC, Stanford University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#define _GNU_SOURCE
#include "kmemory.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define MAX_BLKSIZE	(64*1024)
#define MAX_NBLKS	(16)
#define NBLKS_ALLOC	256
static unsigned long ulBlockSize = 0;
static int iNumBlocks = 0;

void help(char *szProgName)
{
	printf("%s: test application for kmemory Linux module.\n" \
		"Syntax: %s <file1> <file2> \\\n" \
		"\t[ --nblks <numblocks> ] [ --blksz <blocksize> ] \\\n" \
		"\t| --help\n" \
		"Copy data from <file1> to <file2> using kmemory. Note\n" \
		"that using kmemory to do that is only good for testing.\n" \
		"--help: Display this message.\n" \
		"--blksz <blocksize>: size of the buffers used to transfer\n" \
		"\tdata. If 0 then the size is random (and change\n" \
		"\tevery packet).\n" \
		"--nblks <numblocks>: number of blocks to map at a time.\n" \
		"\tIf 0 then the number of blocks is random (and\n" \
		"\tchange every time).\n" \
		"\n", szProgName, szProgName);
}

int copy_file(int fdFileOut, int fdFileIn, int fdKmemory)
{	int fdPipeIn[2], fdPipeOut[2];
	int ret, eof=0;

	if((ulBlockSize == 0) || (iNumBlocks == 0)) {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		printf("Using %u as the random generator seed.\n", tv.tv_sec);
		srandom(tv.tv_sec);
	}
	/* First we open input and output pipe */
	if(pipe(fdPipeIn) < 0) {
		fprintf(stderr, "ERROR: pipe failed, error %s.\n", strerror(errno));
		return -errno;
	}
	if(pipe(fdPipeOut) < 0) {
		fprintf(stderr, "ERROR: pipe failed, error %s.\n", strerror(errno));
		return -errno;
	}
	while(!eof) {
		unsigned long blksize = ulBlockSize == 0 ? (random() % (MAX_BLKSIZE-1))+1 : ulBlockSize;
		int nblkswr = iNumBlocks == 0 ? (random() % (MAX_NBLKS-1))+1 : iNumBlocks;
		struct iovec iovBufIn[NBLKS_ALLOC];
		struct iovec iovBufOut[nblkswr];
		unsigned long sizerd, sizewr;
		int blkrd, blkwr, nblksrd, count, totalrd;

		/* Allocate kmemory buffers */
		if (kmemory_map_write(fdKmemory, iovBufOut, nblkswr, blksize) < 0) {
			fprintf(stderr, "ERROR: kmemory_map_write failed requesting %d bufs of %dB, error %s.\n", 
						nblkswr, blksize, strerror(errno));
			return -errno;
		}
		/* splice ulBlockSize*iNumBlocks data from in to kmemory */
		count = 0;
		totalrd = 0;
		while(totalrd < blksize*nblkswr) {
			if(count<blksize*nblkswr) {
				ret = splice(fdFileIn, NULL, fdPipeIn[1], NULL, blksize*nblkswr-count, SPLICE_F_MOVE);
				if (ret <= 0) {
					if(ret == 0) { 
						eof = 1; 
						break;
					}
					fprintf(stderr, "ERROR: splice (in->pipe) failed, error %s.\n", strerror(errno));
					return -errno;
				}
				count += ret;
			}
			ret = splice(fdPipeIn[0], NULL, fdKmemory, NULL, blksize*nblkswr-totalrd, SPLICE_F_MOVE);
			if (ret <= 0) {
				if(ret == 0) break;
				fprintf(stderr, "ERROR: splice (pipe->kmemory) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			totalrd += ret;
		}
		/* map kmemory buffers */
		ret = kmemory_map_read(fdKmemory, iovBufIn, NBLKS_ALLOC);
		if (ret < 0) {
			fprintf(stderr, "ERROR: kmemory_map_read failed, error %s.\n", strerror(errno));
			return -errno;
		}
		nblksrd = ret;
		/* Copy data into the allocated buffers */
		blkrd = 0;
		blkwr = 0;
		sizerd = 0;
		sizewr = 0;
		count = 0;
		while(count < totalrd) {
			int copylen;

			if (blkrd == nblksrd) {
				fprintf(stderr, "ERROR: not enough input buffer: %d, increase the number.\n",nblksrd);
				return -ENOMEM;
			}
			copylen = iovBufIn[blkrd].iov_len-sizerd < iovBufOut[blkwr].iov_len-sizewr ? 
					iovBufIn[blkrd].iov_len-sizerd : iovBufOut[blkwr].iov_len-sizewr;
			memcpy(iovBufOut[blkwr].iov_base + sizewr, iovBufIn[blkrd].iov_base + sizerd, copylen);
			sizerd += copylen;
			sizewr += copylen;
			count += copylen;
			if (sizerd == iovBufIn[blkrd].iov_len) {
				blkrd++;
				sizerd = 0;
			}
			if (sizewr == iovBufOut[blkwr].iov_len) {
				blkwr++;
				sizewr = 0;
			}
		}
		/* unmap the input and output buffers, we randomly de-allocate one before the other */
		if(random() & 1) {
			ret = kmemory_unmap(fdKmemory, iovBufOut, nblkswr);
			if (ret < 0) {
				fprintf(stderr, "ERROR: kmemory_unmap (out) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			ret = kmemory_unmap(fdKmemory, iovBufIn, nblksrd);
			if (ret < 0) {
				fprintf(stderr, "ERROR: kmemory_unmap (in) failed, error %s.\n", strerror(errno));
				return -errno;
			}
		} else {
			ret = kmemory_unmap(fdKmemory, iovBufIn, nblksrd);
			if (ret < 0) {
				fprintf(stderr, "ERROR: kmemory_unmap (in) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			ret = kmemory_unmap(fdKmemory, iovBufOut, nblkswr);
			if (ret < 0) {
				fprintf(stderr, "ERROR: kmemory_unmap (out) failed, error %s.\n", strerror(errno));
				return -errno;
			}
		}
		/* splice from kmemory to out */
		const int buf_size = 64*1024;
		char buf[buf_size];
		printf("Splicing %d bytes\n",totalrd);
		for(blkwr = 0, count=0; count < totalrd; blkwr++) {
			ret = splice(fdKmemory, NULL, fdPipeOut[1], NULL, totalrd-count, SPLICE_F_MOVE);
			if (ret < 0) {
				fprintf(stderr, "ERROR: splice (kmemory->pipe) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			printf("Spliced %d bytes (kmemory->pipe)\n",ret);
			/***
			printf("Splicing %d bytes pipe(%d)->out(%d)\n",ret,fdPipeOut[0],fdFileOut);
			ret = splice(fdPipeOut[0], NULL, fdFileOut, NULL, ret, SPLICE_F_MOVE);
			if (ret < 0) {
				fprintf(stderr, "ERROR: splice (pipe->out) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			***/
			ret = read (fdPipeOut[0], buf, buf_size);
			printf("Copied %d bytes from pipe(%d)\n", ret, fdPipeOut[0]);
			ret = write(fdFileOut, buf, ret);
			printf("Copied %d bytes to out(%d)\n", ret, fdFileOut);
			count += ret;		
		}
	}
	/* close pipes */
	close(fdPipeIn[0]);
	close(fdPipeIn[1]);
	close(fdPipeOut[0]);
	close(fdPipeOut[1]);
	return 0;
}
 
int main(int argc, char *argv[])
{
	int i, ret;
	int fdFileIn, fdFileOut, fdKmemoryDrv;
	char bWrite = 0;
	char *szFileIn = NULL;
	char *szFileOut = NULL;

	/* Read the command line */
	for(i=1; i<argc; i++) {
		if(strcmp("--help", argv[i]) == 0) {
			help(argv[0]);
			return 0;
		} else if(strcmp("--blksz", argv[i]) == 0) {
			ulBlockSize = atol(argv[++i]);
		} else if(strcmp("--nblks", argv[i]) == 0) {
			iNumBlocks = atoi(argv[++i]);
		} else if(szFileIn == NULL) {
			szFileIn = argv[i];
		} else if(szFileOut == NULL) {
			szFileOut = argv[i];
		} else {
			fprintf(stderr, "ERROR: Invalid argument %s, try --help.\n\n", argv[i]);
			return 1;
		}
	}
	if((szFileIn == NULL) || (szFileOut == NULL)) {
		fprintf(stderr, "ERROR: Missing argument, try --help.\n\n");
		return 2;
	}
	/* Initialize the kmemory file descriptor */
	fdKmemoryDrv = kmemory_open();
	if(fdKmemoryDrv < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n" \
				"Are you sure the kmemory driver is loaded ?\n\n",
				"/dev/" KMEMORY_DEVNAME, strerror(errno));
		return 5;
	}
	/* Open the file to read/write from */
	fdFileIn = open(szFileIn, O_RDONLY | O_NONBLOCK);
	if(fdFileIn < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n", szFileIn, strerror(errno));
		kmemory_close(fdKmemoryDrv);
		return 10;
	}
	fdFileOut = open(szFileOut, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if(fdFileOut < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n", szFileOut, strerror(errno));
		close(fdFileIn);
		kmemory_close(fdKmemoryDrv);
		return 10;
	}
	else
	  printf("Opened %s (%d)\n",szFileOut,fdFileOut);
	/* Now do the real stuff */
	ret = copy_file(fdFileOut, fdFileIn, fdKmemoryDrv);
	if (ret < 0) {
		fprintf(stderr, "Copy failed.\n\n");
		ret = 15;
	} else {
		ret = 0;
	}
	/* We are done, close all the handles and exit */
	close(fdFileIn);
	close(fdFileOut);
	kmemory_close(fdKmemoryDrv);
	return ret;
}
