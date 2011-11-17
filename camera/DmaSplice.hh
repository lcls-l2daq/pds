#ifndef DmaSplice_hh
#define DmaSplice_hh

#include "pds/service/Routine.hh"

namespace Pds {

  class Task;

  class DmaSplice : public Routine {
  public:
    DmaSplice();
    ~DmaSplice();
  public:
    void initialize(void* base, unsigned len);
    void queue     (void* data, unsigned len, unsigned rel_arg);
    int  fd        () const { return _fd; }
  private:
    void routine   (void);
  private:
    int   _fd;
    Task* _task;
  };
};

#endif
