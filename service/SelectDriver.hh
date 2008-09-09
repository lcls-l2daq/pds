#ifndef PDS_SELECTDRIVER_HH
#define PDS_SELECTDRIVER_HH

#include "Routine.hh"

namespace Pds {

class Select;
class Task;

// This is an Routine which can be used to activate an Select in
// an Task.

class SelectDriver: private Routine {
public:
  SelectDriver(Select& select);

  void run(Task& task);

private:
  virtual void routine();

private:
  Select& _select;
};

}
#endif
