#ifndef PDSETHER_HH
#define PDSETHER_HH

namespace Pds {
class Ether {
public:
  Ether();
  Ether(const Ether& ether);
  Ether(const unsigned char* as_array);

  const unsigned char* as_array(unsigned char* buffer) const;
  const char* as_string(char* buffer) const;

private:
  unsigned _mac[2];
};
}
#endif
