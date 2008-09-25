#ifndef Pds_EbJoin_hh
#define Pds_EbJoin_hh

namespace Pds {

  class EbJoin : public Message {
  public:
    EbJoin(int index);
  public:
    int index() const;
  private:
    int _index;
  };

}


inline Pds::EbJoin::EbJoin(int index) : 
  Message(Message::Join, sizeof(EbJoin)),
  _index(index) 
{
}

inline int Pds::EbJoin::index() const 
{
  return _index; 
}

#endif
