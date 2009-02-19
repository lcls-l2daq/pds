#ifndef Pds_MonUSAGE_HH
#define Pds_MonUSAGE_HH

class iovec;

namespace Pds {

  class MonUsage {
  public:
    MonUsage();
    ~MonUsage();

    int use(int signature);
    int dontuse(int signature);

    void reset();
    void request(iovec& iov);

    unsigned short used() const;
    int signature(unsigned short u) const;
    bool ismodified() const;

  private:
    void adjust();

  private:
    int* _signatures;
    unsigned short* _usage;
    unsigned short _used;
    unsigned short _maxused;
    bool _ismodified;
  };
};

#endif
