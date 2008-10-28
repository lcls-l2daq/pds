#ifndef PDSEVGRBOARDINFO_HH
#define PDSEVGRBOARDINFO_HH

#include <semaphore.h>

namespace Pds {
  template <class T> class EvgrBoardInfo {
  public:
    EvgrBoardInfo(const char* dev);
    sem_t* sem()  {return &_sem;}
    int filedes() {return _fd;}
    T& board()    {return *_board;}
  private:
    int _fd;
    T*  _board;
    sem_t _sem;
    char _errmsg[128];
  };

}

#endif
