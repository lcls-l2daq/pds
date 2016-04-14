
#include <sys/mman.h>
#include <new>
#include <fcntl.h>
#include <stdio.h>
#include "EvgrBoardInfo.hh"

using namespace Pds;

template <class T> EvgrBoardInfo<T>::EvgrBoardInfo(const char* dev) {
  sem_init(&_sem, 0, 0);
  _fd = open(dev, O_RDWR);
  if (!_fd)
    {
      printf("Could not open %s\n", dev);
    }
  printf("evr rd %d\n",_fd);

  void* ptr = mmap(0, sizeof(T), PROT_READ | PROT_WRITE,
                   MAP_SHARED, _fd, 0);
  if (ptr == MAP_FAILED) {
    printf("Failed to mmap %s\n",dev);
    perror("mmap");
    _board=0;
  } else {
    printf("Constructing Board\n");
    _board = new(ptr) T;
  }
}

// explicitly instantiate the templates
#include "evgr/evg/evg.hh"
#include "evgr/evr/evr.hh"
template class EvgrBoardInfo<Evg>;
template class EvgrBoardInfo<Evr>;
