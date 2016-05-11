#ifndef Pds_IocOccurrence_hh
#define Pds_IocOccurrence_hh
#include "pds/service/GenericPool.hh"
#include "pds/service/Routine.hh"
#include<string>
#include<list>


namespace Pds {
   class IocOccurrence;
   class IocControl;
   class Task;
   class Semaphore;
}

class Pds::IocOccurrence : public Pds::Routine {
  public:
    IocOccurrence(IocControl *cntl);
    ~IocOccurrence();
    void iocControlError(const std::string& msg, unsigned expt, unsigned run, unsigned stream, unsigned chunk);
    void routine();

  private:
    class Message {
      public:
        Message(const std::string& msg, unsigned expt, unsigned run, unsigned stream, unsigned chunk);
        std::string _msg;
        unsigned    _expt;
        unsigned    _run;
        unsigned    _stream;
        unsigned    _chunk;
    };

  private:
    IocControl* _cntl;
    GenericPool _dataFileErrorPool;
    GenericPool _userMessagePool;
    Task*       _task;
    Semaphore*  _sem;
    std::list<Message*> _messages;
};

#endif
