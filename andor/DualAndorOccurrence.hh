#ifndef __DUALANDOROCCURRENCE_HH
#define __DUALANDOROCCURRENCE_HH

namespace Pds {
   class DualAndorOccurrence;
   class DualAndorManager;
   class GenericPool;
}

class Pds::DualAndorOccurrence {
  public:
    DualAndorOccurrence(DualAndorManager *mgr);
    ~DualAndorOccurrence();
    void outOfOrder(void);
    void userMessage(const char *msgText);

  private:
    DualAndorManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
