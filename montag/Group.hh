#ifndef MonTag_Group_hh
#define MonTag_Group_hh

#include "pds/montag/Tag.hh"

#include <string>
#include <list>

namespace Pds {
  namespace MonTag {
    class Tag;
    class Group {
    public:
      enum { NAME_LEN=32 };
      Group(const std::string& name,
            unsigned           position  =0,
            unsigned           maxmembers=15,
            unsigned           maxbuffers=16);
    public: 
      const std::string& name() const { return _name; }
      Tag                tag (int member) const;
      // member list change
      //
      //  Returns index of new member; -1 on error
      //
      int      insert();
      void     remove(unsigned);
      bool     empty () const { return _members==0; }
    public: 
      // queue tag request
      void     push(unsigned member,
                    unsigned buffer);
      // next tag request, if any
      unsigned pop ();
    private:
      std::string         _name;
      unsigned            _position;
      unsigned            _max_members;
      unsigned            _member_bits;
      unsigned            _buffer_bits;
      unsigned            _no_request;
      unsigned            _members;
      std::list<unsigned> _requests;
    };
  };
};

#endif
