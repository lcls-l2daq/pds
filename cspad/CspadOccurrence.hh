#ifndef __CSPADOCCURRENCE_HH
#define __CSPADOCCURRENCE_HH

namespace Pds {
   class CspadOccurrence;
   class CspadManager;
   class GenericPool;
}

class Pds::CspadOccurrence {
  public:
    CspadOccurrence(CspadManager *mgr);
    ~CspadOccurrence();
    void outOfOrder(void);
    void userMessage(char *msgText);

  private:
    CspadManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
