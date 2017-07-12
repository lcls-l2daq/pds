#ifndef MonTag_Node_hh
#define MonTag_Node_hh

namespace Pds {
  class Ins;
  namespace MonTag {
    class Group;

    class Node {
    public:
      Node(int        socket,
           int        member,
           Group&     group);
      ~Node();
    public: 
      int      fd    () const { return _socket; }
      int      member() const { return _member; }
      Group&   group () const { return _group; }
    private:
      int    _socket;
      int    _member;
      Group& _group;
    };
  };
};

#endif
