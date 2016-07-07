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
    void iocControlWarning(const std::string& msg);
    void routine();

  private:
    class Message {
      friend class IocOccurrence;
      public:
        Message(const std::string& msg);
        Message(const std::string& msg, unsigned expt, unsigned run, unsigned stream, unsigned chunk);
      private:
        std::string _msg;
        unsigned    _expt;
        unsigned    _run;
        unsigned    _stream;
        unsigned    _chunk;
        bool        _error;
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
