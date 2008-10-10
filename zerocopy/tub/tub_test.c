#define _GNU_SOURCE
#include "tub.h"
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
	printf("%s: test application for tub module.\n" \
		"Syntax: %s <file1> <file2> \\\n" \
		"\t[ --nblks <numblocks> ] [ --blksz <blocksize> ] \\\n" \
		"\t| --help\n" \
		"Copy data from <file1> to <file2> using tub. Note\n" \
		"that using tub to do that is only good for testing.\n" \
		"--help: Display this message.\n" \
		"--blksz <blocksize>: size of the buffers used to transfer\n" \
		"\tdata. If 0 then the size is random (and change\n" \
		"\tevery packet).\n" \
		"--nblks <numblocks>: number of blocks to map at a time.\n" \
		"\tIf 0 then the number of blocks is random (and\n" \
		"\tchange every time).\n" \
		"\n", szProgName, szProgName);
}

int copy_file(int fdFileOut, int fdFileIn, int fdTub)
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
	else {
	        fprintf(stderr, "PASS: pipe-in created %d/%d.\n", fdPipeIn[0],fdPipeIn[1]);
	}
	if(pipe(fdPipeOut) < 0) {
		fprintf(stderr, "ERROR: pipe failed, error %s.\n", strerror(errno));
		return -errno;
	}
	else {
	        fprintf(stderr, "PASS: pipe-out created %d/%d.\n", fdPipeOut[0],fdPipeOut[1]);
	}
	while(!eof) {
		unsigned long blksize = ulBlockSize == 0 ? (random() % (MAX_BLKSIZE-1))+1 : ulBlockSize;
		int nblkswr = iNumBlocks == 0 ? (random() % (MAX_NBLKS-1))+1 : iNumBlocks;
		struct iovec iovBufIn[NBLKS_ALLOC];
		struct iovec iovBufOut[nblkswr];
		unsigned long sizerd, sizewr;
		int blkrd, blkwr, nblksrd, count, totalrd;

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
			ret = splice(fdPipeIn[0], NULL, fdTub, NULL, blksize*nblkswr-totalrd, SPLICE_F_MOVE);
			if (ret <= 0) {
				if(ret == 0) break;
				fprintf(stderr, "ERROR: splice (pipe->tub) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			else {
			  fprintf(stderr,"PASS: splice (pipe->tub) %d bytes\n",ret);
			}
			totalrd += ret;
		}
		for(blkwr = 0, count=0; count < totalrd; blkwr++) {
			ret = splice(fdTub, NULL, fdPipeOut[1], NULL, totalrd-count, SPLICE_F_MOVE);
			if (ret < 0) {
				fprintf(stderr, "ERROR: splice (tub->pipe) failed, error %s.\n", strerror(errno));
				return -errno;
			}
			else {
			  fprintf(stderr,"PASS: splice (tub->pipe) %d bytes\n",ret);
			}
			ret = splice(fdPipeOut[0], NULL, fdFileOut, NULL, ret, SPLICE_F_MOVE);
			if (ret < 0) {
				fprintf(stderr, "ERROR: splice (pipe->out) failed, error %s.\n", strerror(errno));
				return -errno;
			}
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
	int fdFileIn, fdFileOut, fdTub;
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
	fdTub = tub_open();
	if(fdTub < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n" \
				"Are you sure the tub driver is loaded ?\n\n",
				"/dev/" TUB_DEVNAME, strerror(errno));
		return 5;
	}
	else {
	        fprintf(stderr, "PASS: Opened tub %s, %d.\n", "/dev/" TUB_DEVNAME, fdTub);
	}
	/* Open the file to read/write from */
	fdFileIn = open(szFileIn, O_RDONLY | O_NONBLOCK);
	if(fdFileIn < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n", szFileIn, strerror(errno));
		tub_close(fdTub);
		return 10;
	}
	else {
	        fprintf(stderr, "PASS: Opened %s, %d.\n", szFileIn, fdFileIn);
	}
	fdFileOut = open(szFileOut, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR);
	if(fdFileOut < 0) {
		fprintf(stderr, "ERROR: Could not open %s, %s.\n", szFileOut, strerror(errno));
		close(fdFileIn);
		tub_close(fdTub);
		return 10;
	}
	else {
	        fprintf(stderr, "PASS: Opened %s, %d.\n", szFileOut, fdFileOut);
	}
	/* Now do the real stuff */
	ret = copy_file(fdFileOut, fdFileIn, fdTub);
	if (ret < 0) {
		fprintf(stderr, "Copy failed.\n\n");
		ret = 15;
	} else {
		ret = 0;
	}
	/* We are done, close all the handles and exit */
	close(fdFileIn);
	close(fdFileOut);
	tub_close(fdTub);
	return ret;
}
