#ifndef Pds_MonENTRY_HH
#define Pds_MonENTRY_HH

class iovec;

namespace Pds {

  class ClockTime;
  class MonDescEntry;

  class MonEntry {
  public:
    MonEntry();
    virtual ~MonEntry();

    virtual MonDescEntry& desc() = 0;
    virtual const MonDescEntry& desc() const = 0;
    virtual int update() {return 0;}

    void payload(iovec& iov);
    void payload(iovec& iov) const;

    double last() const;
    const ClockTime& time() const;
    void time(const ClockTime& t);

    void reset();

  protected:
    void* allocate(unsigned size);

  private:
    unsigned _payloadsize;
    unsigned long long* _payload;
  };
};

#endif

  
