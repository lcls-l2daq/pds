#include "pds/client/QueuedAction.hh"
#include "pds/client/Action.hh"
#include "pds/service/Semaphore.hh"

using namespace Pds;

    void QueuedAction::routine() {
	_rec.fire(_in);

      

      if (_sem)	_sem->give();
      delete this;
    }
