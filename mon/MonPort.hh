#ifndef Pds_MonPORT_HH
#define Pds_MonPORT_HH

namespace Pds {

  class MonPort {
  public:
    enum Type {Mon, Vmon, Dhp, Vtx, Fct, Test, NTypes};
    
    static unsigned short port(Type type);
    static const char* name(Type type);
    static Type type(const char* name);
    
  };
};

#endif
