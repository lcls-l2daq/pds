#ifndef __ENCODEROCCURRENCE_HH
#define __ENCODEROCCURRENCE_HH

namespace Pds {
   class EncoderOccurrence;
   class EncoderManager;
   class GenericPool;
}

class Pds::EncoderOccurrence {
  public:
    EncoderOccurrence(EncoderManager *mgr);
    ~EncoderOccurrence();
    void outOfOrder(void);
    void userMessage(char *msgText);

  private:
    EncoderManager* _mgr;
    GenericPool* _outOfOrderPool;
    GenericPool* _userMessagePool;
};

#endif
