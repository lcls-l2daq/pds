#include <stdlib.h>
#include <unistd.h>

#include "pds/service/Task.hh"
#include "pds/utility/Occurrence.hh"
#include "pds/utility/OccurrenceId.hh"

#include "IocControl.hh"
#include "IocOccurrence.hh"

using namespace Pds;

IocOccurrence::IocOccurrence(IocControl *cntl) :
  _cntl      (cntl),
  _dataFileErrorPool(sizeof(DataFileError),4),
  _userMessagePool  (sizeof(UserMessage),4),
  _task             (new Task(TaskObject("IocOccurance"))),
  _sem              (new Semaphore(Semaphore::FULL))
{
}

IocOccurrence::~IocOccurrence()
{
  _task->destroy();
  _messages.clear();
  delete _sem;
}

void IocOccurrence::iocControlError(const std::string& msg, unsigned expt, unsigned run, unsigned stream, unsigned chunk)
{
  _sem->take();
  _messages.push_back(new Message(msg, expt, run, stream, chunk));
  _sem->give();
  _task->call(this);
}

void IocOccurrence::routine()
{
  Message* errm = 0;
  // See if there are any messages to process
  _sem->take();
  if(!_messages.empty()) {
    errm = _messages.front();
    _messages.pop_front();
  }
  _sem->give();

  if(errm) {
    // send occurrence: file error
    DataFileError* occ = new (&_dataFileErrorPool) DataFileError(errm->_expt, errm->_run, errm->_stream, errm->_chunk);
    _cntl->post(occ);
    // send occurrence: user message
    UserMessage* msg = new (&_userMessagePool) UserMessage(errm->_msg.c_str());
    _cntl->post(msg);

    delete errm;
  }
}

IocOccurrence::Message::Message(const std::string& msg, unsigned expt, unsigned run, unsigned stream, unsigned chunk) :
  _msg(msg),
  _expt(expt),
  _run(run),
  _stream(stream),
  _chunk(chunk)
{}
