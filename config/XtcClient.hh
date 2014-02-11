#ifndef Pds_XtcClient_hh
#define Pds_XtcClient_hh

namespace Pds { class Src; class TypeId; };

namespace Pds_ConfigDb {
  class XtcClient {
  public:
    virtual ~XtcClient() {}
  public:
    static XtcClient* open  ( const char* path );
  public:
    virtual int       getXTC( unsigned           key,
                              const Pds::Src&    src,
                              const Pds::TypeId& type_id,
                              void*              dst,
                              unsigned           maxSize) = 0;
  };
};

#endif
