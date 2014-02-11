#ifndef Pds_XtcClientNfs_hh
#define Pds_XtcClientNfs_hh

#include "pds/config/XtcClient.hh"

#include <string>

namespace Pds_ConfigDb {
  namespace Nfs {
    class XtcClient : public Pds_ConfigDb::XtcClient {
    private:
      XtcClient(const char*);
    public:
      ~XtcClient();
    public:
      static XtcClient* open ( const char* path );
    public:
      int       getXTC( unsigned           key,
                        const Pds::Src&    src,
                        const Pds::TypeId& type_id,
                        void*              dst,
                        unsigned           maxSize);
    private:
      std::string _path;
    };
  };
};

#endif
