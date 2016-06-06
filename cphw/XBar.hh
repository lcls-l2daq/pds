#ifndef Pds_Cphw_XBar_hh
#define Pds_Cphw_XBar_hh

namespace Pds {
  namespace Cphw {
    template <class T>
    class XBar {
    public:
      enum InMode  { StraightIn , LoopIn };
      enum OutMode { StraightOut, LoopOut };
      void setEvr( InMode  m );
      void setEvr( OutMode m );
      void setTpr( InMode  m );
      void setTpr( OutMode m );
  void dump() const;
    public:
      T outMap[4];
    };
  }
}

#endif
