#ifndef PdsTag_Key_hh
#define PdsTag_Key_hh

static const unsigned MAX_GROUPS = 2;
static const unsigned MAX_GROUP_SIZE = 16;

#include <stdint.h>

namespace Pds {
  namespace Tag {
    class Key {
    public:
      enum Option {Invalid};
      Key() {}
      Key(Option);
      Key(unsigned platform,
          int      group,
          unsigned member);
    public:
      bool     valid   () const;
      unsigned platform() const;
      int      group () const;
      unsigned member() const;
      uint32_t mask  () const;
      uint32_t tag   (unsigned buf) const;
      int      buffer(uint32_t tag) const;
    private:
      uint8_t  _platform;
      uint8_t  _member;
      int16_t  _group;
    };
  };
};

#endif
