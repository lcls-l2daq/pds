#include <string.h>

#include "pds/mon/MonPoll.hh"
#include "pds/mon/MonFd.hh"

static const unsigned short Step = 32;

using namespace Pds;

MonPoll::MonPoll(int timeout) :
  MonLoopback(),
  _timeout(timeout < 0 ? -1 : timeout),
  _nfds(1),
  _maxfds(Step),
  _ofd(new MonFd*[_maxfds]),
  _pfd(new pollfd[_maxfds])
{
  _ofd[0] = 0;
  _pfd[0].fd = socket();
  _pfd[0].events = POLLIN;
  _pfd[0].revents = 0;
}

MonPoll::~MonPoll() 
{
  delete [] _ofd;
  delete [] _pfd;
}

int MonPoll::manage(MonFd& fd) 
{
  unsigned available = 0;
  for (unsigned short n=1; n<_nfds; n++) {
    if (!_ofd[n] && !available) available = n;
    if (_ofd[n] == &fd) return -1;
  }
  if (!available) {
    if (_nfds == _maxfds) adjust();
    available = _nfds++;
  }
  _ofd[available] = &fd;
  _pfd[available].fd = fd.fd();
  _pfd[available].events = POLLIN;
  _pfd[available].revents = 0;
  return 0;
}

int MonPoll::unmanage(MonFd& fd)
{
  for (unsigned short n=1; n<_nfds; n++) {
    if (_ofd[n] == &fd) {
      _ofd[n] = 0;
      _pfd[n].fd = -1;
      _pfd[n].events = 0;
      _pfd[n].revents = 0;
      return 0;
    }
  }
  return -1;
}

int MonPoll::timeout() const {return _timeout;}
void MonPoll::dotimeout(int timeout) {_timeout = timeout < 0 ? -1 : timeout;}
void MonPoll::donottimeout() {_timeout = -1;}

#include <stdio.h>

int MonPoll::poll()
{
  if (::poll(_pfd, _nfds, _timeout) > 0) {
    if (_pfd[0].revents & (POLLIN | POLLERR)) {
      if (!processMsg()) return 0;
    }
    bool processfd = false;
    for (unsigned short n=1; n<_nfds; n++) {
      if (_ofd[n]) {
	if (_pfd[n].revents & (POLLIN | POLLERR)) {
	  if (!_ofd[n]->processIo()) {
	    if (!processFd(*_ofd[n])) processfd = true;
	  }
	}
      }
    }
    if (processfd && !processFd()) return 0;
    return 1;
  } else {
    return processTmo();
  }
}

void MonPoll::shrink()
{
  unsigned short nfds = 1;
  for (unsigned short n=1; n<_nfds; n++) {
    if (_ofd[n]) {
      _ofd[nfds] = _ofd[n];
      _pfd[nfds].fd = _pfd[n].fd;
      _pfd[nfds].events = _pfd[n].events;
      nfds++;
    }
  }
  _nfds = nfds;
}

void MonPoll::adjust()
{
  unsigned short maxfds = _maxfds + Step;

  MonFd** ofd = new MonFd*[maxfds];
  memcpy(ofd, _ofd, _nfds*sizeof(MonFd*));
  delete [] _ofd;
  _ofd = ofd;

  pollfd* pfd = new pollfd[maxfds];
  memcpy(pfd, _pfd, _nfds*sizeof(pollfd));
  delete [] _pfd;
  _pfd = pfd;

  _maxfds = maxfds;
}
