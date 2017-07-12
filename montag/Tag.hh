#ifndef MonTag_Tag_hh
#define MonTag_Tag_hh

namespace Pds {
  class Env;
  namespace MonTag {
    class Tag {
    public:
      Tag() {}
      Tag(unsigned position,
          unsigned memberbits,
          unsigned bufferbits,
          unsigned member);
    public:
      //  Returns buffer for event forwarding; -1 if not requested
      int      buffer(const Env&) const;
    public:
      static unsigned request(unsigned position,
                              unsigned memberbits,
                              unsigned bufferbits,
                              unsigned member,
                              unsigned buffer);
    private:
      unsigned _position;
      unsigned _member_bits;
      unsigned _buffer_bits;
      unsigned _member;
    };
  };
};

#endif
