// $Id$
// Author: Chris Ford <caf@slac.stanford.edu>

#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

struct termios oldsettings, newsettings;

void makeraw(int fd)
{
  if (tcgetattr(fd, &oldsettings)) {
    perror("tcgetattr");
  } else {
    newsettings = oldsettings;
    cfmakeraw(&newsettings);
    tcsetattr(fd, TCSANOW, &newsettings);
  }
}

#include "rayonix_data.hh"

#define BUFSIZE (32 * 1024 * 1024)

#define DOT_STRIDE  10
#define LINE_STRIDE  100

void help(char *prog_name)
{
  printf("usage: %s [--verbose] [--write]\n\r", prog_name);
}

int writeFrame(char *buf, int pixels, uint16_t frameNumber)
{
  FILE *ff;
  int ss;
  char fname[256];
  int rv = 0;

  sprintf(fname,"rayonix-f%06d.bin", frameNumber);
  ff = fopen(fname,"wx"); // x: if the file already exists, fopen() fails
  if (ff) {
    ss = fwrite(buf, sizeof(uint16_t), pixels, ff);
    if (ss != pixels) {
      printf("ERROR: fwrite() returned %d, expected %d\n\r", ss, pixels);
      rv = -1;
    } else {
      printf("Writing frame #%d to file %s complete.\n\r", frameNumber, fname);
    }
  } else {
    perror("fopen");
    rv = -1;
  }

  return (rv);
}

void dodots(uint16_t num)
{
  static bool firstCall = true;

  if (firstCall) {
    printf("[frame %hu]\n\r", num);
    firstCall = false;
  } else {
    if (num % DOT_STRIDE == 0) {
      printf("."); fflush(stdout);
    }
    if (num % LINE_STRIDE == 0) {
      printf(" [frame %hu]\n\r", num);
    }
  }
}

int main(int argc, char *argv[])
{
  int i, rv;
  uint16_t frameNumber;
  int binning_f, binning_s;
  bool verbose = false;
  Pds::rayonix_data *pData;
  char *buf;
  fd_set notify_set;
  int notifyFd, nfds;
  bool quitFlag = false;
  bool writeFlag = false;

  // parse arguments
  for(i=1; i<argc; i++) {
    if(strcmp(argv[i], "--help") == 0) {
      help(argv[0]);
      return 0;
    } else if(strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if(strcmp(argv[i], "--write") == 0) {
      writeFlag = true;
    } else {
      printf("%s: invalid option -- '%s'\n\rTry %s --help for more information.\n\r",
             argv[0], argv[i], argv[0]);
      return 1;
    }
  }

  // init
  makeraw(0);
  buf = new char[BUFSIZE];
  pData = new Pds::rayonix_data(10000, verbose);
  notifyFd = pData->fd();
  nfds = notifyFd + 1;
  pData->reset(verbose);

  if (verbose) {
    printf("Type 'w' to write frame, 'q' to quit, 'v' to toggle verbose flag\n\r");
    printf("Entering read loop...\n\r");
  }

  do {
    // select
    FD_ZERO(&notify_set);
    FD_SET(notifyFd, &notify_set);
    FD_SET(0, &notify_set);         // stdin

    if (select(nfds, &notify_set, NULL, NULL, NULL) == -1) {
      perror("select");
      rv = -1;
    } else {
      if FD_ISSET(0, &notify_set) {
        // stdin
        int key = tolower(getchar());
        if ((3 == key) || ('q' == key)) {
          quitFlag = true;
          printf(" [quit]\n\r"); fflush(stdout);
        }
        if ('v' == key) {
          verbose = !verbose;
          printf(" [verbose=%s]\n\r", verbose ? "true" : "false"); fflush(stdout);
        }
        if ('w' == key) {
          writeFlag = true;
          printf(" [write next frame]\n\r"); fflush(stdout);
        }
      }
      if FD_ISSET(notifyFd, &notify_set) {
        // data socket
        rv = pData->readFrame(frameNumber, buf, BUFSIZE, binning_f, binning_s, verbose);
        if (!verbose) {
          dodots(frameNumber);
        }
        if (writeFlag) {
          quitFlag = true;    // quit after writing
          if (rv > 0) {
            printf("Write frame #%d (%d pixels) to file...\n\r", frameNumber, rv);
            if (writeFrame(buf, rv, frameNumber) == -1) {
              printf("ERROR: writeFrame() failed for frame #%d\n\r", frameNumber);
            }
          } else {
            printf("ERROR: readFrame() failed for frame #%d\n\r", frameNumber);
          }
        }
      }
    }
  } while (!quitFlag && (rv >= 0));

  if (verbose) {
    printf("Exited read loop.\n\r");
  }

  // fini
  delete[] buf;
  delete pData;
}
