#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "Arp.hh"
#include "Ether.hh"

using namespace Pds;

Arp::Arp(const char* suidprocess) :
  _pid(0),
  _error(0)
{
  memset(_added, 0, 32*sizeof(unsigned));
  if (pipe(_fd1) < 0 || pipe(_fd2) < 0) {
    _error = errno;
    return;
  }
  if ((_pid = fork()) < 0) {
    _error = errno;
    return;
  } else if (_pid > 0) { 
    // parent
    close(_fd1[0]);
    close(_fd2[1]);
    char buf[64];
    int len = read(_fd2[0], buf, sizeof(buf));
    if (len < 0) {
      _error = errno;
    } else {
      buf[len] = 0;
      sscanf(buf, "%d", &_error);
    }
    return;
  } else { 
    // child
    close(_fd1[1]);    
    close(_fd2[0]);
    char buf[64];
    if (_fd1[0] != STDIN_FILENO) {
      if (dup2(_fd1[0], STDIN_FILENO) < 0) {
	sprintf(buf, "%d", errno);
	write(_fd2[1], buf, strlen(buf));
	_exit(errno);
      }
    }
    if (_fd2[1] != STDOUT_FILENO) {
      if (dup2(_fd2[1], STDOUT_FILENO) < 0) {
	sprintf(buf, "%d", errno);
	write(_fd2[1], buf, strlen(buf));
	_exit(errno);
      }
    }
    execl(suidprocess, suidprocess, (char*)0);
    sprintf(buf, "%d", errno);
    write(_fd2[1], buf, strlen(buf));
    _exit(errno);
  }
}

Arp::~Arp()
{
  if (!_error) {
    char buf[2];
    buf[0] = 1;
    buf[1] = 0;
    if (write(_fd1[1], buf, strlen(buf)) < 0) return;
  }
  close(_fd1[1]);
  close(_fd2[0]);
  if (_pid) waitpid(_pid, NULL, 0);
}

int Arp::add(const char* dev, int ip, const Ether& ether)
{
  char hwaddr[20];
  ether.as_string(hwaddr);
  char buf[64];
  snprintf(buf, sizeof(buf), "%d.%d.%d.%d %s %s", 
	   (ip>>24)&0xff, (ip>>16)&0xff, (ip>>8)&0xff, ip&0xff, hwaddr, dev);
  int len = strlen(buf);
  len = write(_fd1[1], buf, len);
  if (len < 0) return errno;
  len = read(_fd2[0], buf, sizeof(buf));
  if (len < 0) return errno;
  buf[len] = 0;
  int error;
  sscanf(buf, "%d", &error);
  return error;
}

int Arp::error() const {return _error;}
